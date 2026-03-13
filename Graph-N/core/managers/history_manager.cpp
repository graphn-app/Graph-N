
#include "core/managers/history_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "tools/blob_serializer.h"

namespace mngr
{

/////////////////////////////////////////////////////

HistoryStep::HistoryStep(HistoryActionType t)
    : type(t)
{}
HistoryStep::~HistoryStep() = default;

HistoryAction::HistoryAction(int64_t id_, wid wf_)
    : id(id_)
    , wf(wf_)
{}
HistoryAction::~HistoryAction() = default;
HistoryAction::HistoryAction(HistoryAction&& other)
{
    *this = std::move(other);
}
HistoryAction& HistoryAction::operator=(HistoryAction&& other)
{
    if (this == &other)
        return *this;
    id = other.id;
    wf = other.wf;
    name = std::move(other.name);
    multi_action_counter = other.multi_action_counter;
    steps = std::move(other.steps);
    return *this;
}

////////////////////////////////////////////////////

HistoryManager::HistoryManager()
    : m_limit(200)
{
}

HistoryManager::~HistoryManager()
{
    LogF;
}

bool HistoryManager::init()
{
    LogF;
    if (!Manager::init())
        return false;
    return true;
}

void HistoryManager::free()
{
    LogF;
}

void HistoryManager::stop_all_threads()
{
    LogF;
}

/////   interface
//////////////////////////////////////

void HistoryManager::cleanup(wid wf_id)
{
    m_pos.erase(wf_id);
    m_history.erase(wf_id);
    Log("History cleaned up for wf " << wf_id);
}

const HistoryAction* HistoryManager::get_undo(wid wf_id, const size_t** hpos) const
{
    auto pos = m_pos.find(wf_id);
    if (pos == m_pos.end() || pos->second == 0)
        return nullptr;
    auto h = m_history.find(wf_id);
    if (h == m_history.end()) { LogAndThrowInternalError(ExcepCategory::Logic, "History not exists for wf " << wf_id); }
    if (h->second.size() == 1 || pos->second >= h->second.size()) { LogAndThrowInternalError(ExcepCategory::Logic, "Invalid history pos for wf " << wf_id); }

    auto& undo = h->second[pos->second];
    if (hpos != nullptr)
        *hpos = &pos->second;
    return &undo;
}

const HistoryAction* HistoryManager::get_redo(wid wf_id, const size_t** hpos) const
{
    auto pos = m_pos.find(wf_id);
    if (pos == m_pos.end())
        return nullptr;

    auto h = m_history.find(wf_id);
    if (h == m_history.end()) { LogAndThrowInternalError(ExcepCategory::Logic, "History not exists for wf " << wf_id); }

    if (h->second.size() == 1 || pos->second >= h->second.size() - 1)
        return nullptr;

    auto& redo = h->second[pos->second + 1];
    if (hpos != nullptr)
        *hpos = &pos->second;
    return &redo;
}

const char* HistoryManager::undo_name(wid wf_id) const
{
    auto h = get_undo(wf_id, nullptr);
    if (h != nullptr)
        return h->name.c_str();
    return nullptr;
}

const char* HistoryManager::redo_name(wid wf_id) const
{
    auto h = get_redo(wf_id, nullptr);
    if (h != nullptr)
        return h->name.c_str();
    return nullptr;
}

void HistoryManager::undo(wid wf_id)
{
    if (m_rollback) { LogAndThrowInternalError(ExcepCategory::Logic, "Already in progress"); }
    m_rollback = true;

    const size_t* pos = nullptr;
    auto h = get_undo(wf_id, &pos);
    if (nullptr == h) { LogAndThrowInternalError(ExcepCategory::Logic, "Can't undo"); }
    pop_action(h, true);
    (*const_cast<size_t*>(pos))--;
    m_rollback = false;
}

void HistoryManager::redo(wid wf_id)
{
    if (m_rollback) { LogAndThrowInternalError(ExcepCategory::Logic, "Already in progress"); }
    m_rollback = true;

    const size_t* pos = nullptr;
    auto h = get_redo(wf_id, &pos);
    if (nullptr == h) { LogAndThrowInternalError(ExcepCategory::Logic, "Can't redo"); }
    pop_action(h, false);
    (*const_cast<size_t*>(pos))++;
    m_rollback = false;
}

/////   save action
//////////////////////////////////////

HistoryAction* HistoryManager::open_action(wid wf_id, const char* action_name)
{
    HistoryAction* ret = nullptr;
    hqueue& h = m_history[wf_id];
    size_t& pos = m_pos[wf_id];

    const auto push_new = [action_name, &h, &pos, wf_id, this]()
    {
        HistoryAction act(++m_last_id, wf_id);
        act.name = action_name;
        act.multi_action_counter++;
        h.push_back(std::move(act));
        pos = h.size() - 1;
        return &h.back();
    };

    if (h.empty())
    {
        ret = push_new();
        LogAction(*ret, "Opened", pos, h.size());
    }
    else
    {
        auto& act = h.back();
        if (act.multi_action_counter > 0)
        {
            act.multi_action_counter++;
            ret = &act;
            LogAction(*ret, "Continuing", pos, h.size());
        }
        else
        {
            const auto sz = h.size();
            while(h.size() - 1 > pos)
            {
                h.pop_back();
            }
            if (sz != h.size())
            {
                Log("Wf " << wf_id << ", removed " << sz - h.size() << " actions on back");
            }

            ret = push_new();
            LogAction(*ret, "Opened", pos, h.size());
        }
    }

    const auto sz = h.size();
    while (h.size() > m_limit)
    {
        h.pop_front();
        if (pos > 0)
            pos--;
    }
    if (sz != h.size())
    {
        Log("Wf " << wf_id << ", removed " << sz - h.size() << " actions on front");
    }

    return ret;
}

void HistoryManager::close_action(wid wf_id)
{
    hqueue& h = m_history[wf_id];
    auto& act = h.back();
    if (--act.multi_action_counter > 0)
    {
        LogAction(act, "Pending Close");
        return;
    }

    if (act.steps.empty())
    {
        LogAction(act, "Dropped (Empty)");
        h.pop_back();
    }
    else
    {
        LogAction(act, "Closed", m_pos[wf_id], h.size());
    }
    m_pos[wf_id] = h.size() - 1;
}

void HistoryManager::multi_action_begin(wid wf_id, const char* action_name)
{
    if (m_rollback) return;
    open_action(wf_id, action_name);
}

void HistoryManager::multi_action_end(wid wf_id)
{
    if (m_rollback) return;
    close_action(wf_id);
}

void HistoryManager::push_node_action(const data::NodeInstance* node, HistoryActionType act)
{
    if (m_rollback) return;

    HistoryAction* history = nullptr;
    std::unique_ptr<ActNodeBlob> step;

    switch (act)
    {
    case HistoryActionType::NodeAdd:
        history = open_action(node->get_workflow_id(), node->m_group ? "Add group" : "Add node");
        step = std::make_unique<ActNodeAdd>();
        break;
    case HistoryActionType::NodeDel:
        history = open_action(node->get_workflow_id(), node->m_group ? "Remove group" : "Remove node");
        step = std::make_unique<ActNodeDel>();
        break;
    default:
        { LogAndThrowInternalError(ExcepCategory::Logic, "Invalid action pushed"); }
    }

    if (history->wf != node->get_workflow_id()) { LogAndThrowInternalError(ExcepCategory::Logic, "Wf don't match"); }
    step->id = node->get_instance_id();
    step->debug_name = node->get_name();
    step->node = node->serialize().release();
    history->steps.push_back(std::move(step));

    close_action(node->get_workflow_id());
}

void HistoryManager::push_node_action(const data::NodeInstance* node, HistoryActionType act, HActionT&& old_val)
{
    if (m_rollback) return;

    HistoryAction* history = nullptr;
    std::unique_ptr<ActOldNew> step;

    switch (act)
    {
    case HistoryActionType::NodeClr:
        history = open_action(node->get_workflow_id(), "Set color");
        step = std::make_unique<ActNodeClr>();
        step->new_val = (node->m_group ? node->m_clr_group : ImGui::ColorConvertFloat4ToU32(node->m_clr_node));
        break;
    case HistoryActionType::NodeMode:
        history = open_action(node->get_workflow_id(), "Set mode");
        step = std::make_unique<ActNodeMode>();
        step->new_val = node->get_mode();
        break;
    case HistoryActionType::NodePin:
        history = open_action(node->get_workflow_id(), "Pin node");
        step = std::make_unique<ActNodePin>();
        step->new_val = node->pinned();
        break;
    case HistoryActionType::NodeRect:
        history = open_action(node->get_workflow_id(), "Positioning");
        step = std::make_unique<ActNodeRect>();
        step->new_val = node->m_rc;
        break;
    case HistoryActionType::NodeRen:
        history = open_action(node->get_workflow_id(), "Rename");
        step = std::make_unique<ActNodeRen>();
        step->new_val = node->get_name();
        break;
    default:
        { LogAndThrowInternalError(ExcepCategory::Logic, "Invalid action pushed"); }
    }

    if (history->wf != node->get_workflow_id()) { LogAndThrowInternalError(ExcepCategory::Logic, "Wf don't match"); }
    step->id = node->get_instance_id();
    step->old_val = std::move(old_val);
    history->steps.push_back(std::move(step));

    close_action(node->get_workflow_id());
}

void HistoryManager::push_pin_action(const data::PinInstance* pin, HistoryActionType act, HActionT&& old_val)
{
    if (m_rollback) return;

    HistoryAction* history = nullptr;
    std::unique_ptr<ActOldNew> step;

    if (HistoryActionType::PinRen == act)
    { 
        history = open_action(pin->get_workflow_id(), "Rename");
        step = std::make_unique<ActPinRen>();
        step->new_val = pin->state().name;
    }
    else if (HistoryActionType::PinVal == act)
    { 
        history = open_action(pin->get_workflow_id(), "Set value");
        step = std::make_unique<ActPinVal>();
        step->new_val = pin->state().value;
    }
    else
    { LogAndThrowInternalError(ExcepCategory::Logic, "Invalid action pushed"); }

    if (history->wf != pin->get_workflow_id()) { LogAndThrowInternalError(ExcepCategory::Logic, "Wf don't match"); }
    step->id = pin->get_instance_id();
    step->old_val = std::move(old_val);
    history->steps.push_back(std::move(step));

    close_action(pin->get_workflow_id());
}

void HistoryManager::push_link_action(const data::Link* link, HistoryActionType act)
{
    if (m_rollback) return;

    HistoryAction* history = nullptr;
    std::unique_ptr<ActLink> step;

    if (HistoryActionType::LinkAdd == act)
    { 
        history = open_action(link->get_workflow_id(), "Link");
        step = std::make_unique<ActLinkAdd>();
    }
    else if (HistoryActionType::LinkDel == act)
    { 
        history = open_action(link->get_workflow_id(), "Unlink");
        step = std::make_unique<ActLinkDel>();
    }
    else
    { LogAndThrowInternalError(ExcepCategory::Logic, "Invalid action pushed"); }

    if (history->wf != link->get_workflow_id()) { LogAndThrowInternalError(ExcepCategory::Logic, "Wf don't match");  }
    step->id = link->get_instance_id();
    step->in = link->get_input()->get_instance_id();
    step->out = link->get_output()->get_instance_id();
    history->steps.push_back(std::move(step));

    close_action(link->get_workflow_id());
}

////////  undo / redo
//////////////////////////////////////

void HistoryManager::pop_action(const HistoryAction* action, bool do_undo)
{
    LogAction(*action, do_undo ? "UNDO" : "REDO");
    data::Workflow* wf = g::pWfsManager->access(action->wf);

    if (do_undo)
    {
        for (auto it = action->steps.rbegin(); it != action->steps.rend(); ++it)
        {
            auto ptr = it->get();
            LogStep(ptr, true);
            pop_action(wf, ptr, do_undo);
        }
    }
    else
    {
        for (auto& step : action->steps)
        {
            auto ptr = step.get();
            LogStep(ptr, false);
            pop_action(wf, ptr, do_undo);
        }
    }

    LogAction(*action, "DONE");
}

void HistoryManager::pop_action(data::Workflow* wf, const HistoryStep* step, bool do_undo)
{
    switch (step->type)
    {
    case HistoryActionType::NodeAdd:
        pop_node_add(wf, step, do_undo);
        break;
    case HistoryActionType::NodeDel:
        pop_node_del(wf, step, do_undo);
        break;
    case HistoryActionType::NodeClr:
        pop_node_clr(wf, step, do_undo);
        break;
    case HistoryActionType::NodeMode:
        pop_node_mode(wf, step, do_undo);
        break;
    case HistoryActionType::NodePin:
        pop_node_pin(wf, step, do_undo);
        break;
    case HistoryActionType::NodeRect:
        pop_node_rect(wf, step, do_undo);
        break;
    case HistoryActionType::NodeRen:
        pop_node_ren(wf, step, do_undo);
        break;
    case HistoryActionType::PinRen:
        pop_pin_ren(wf, step, do_undo);
        break;
    case HistoryActionType::PinVal:
        pop_pin_val(wf, step, do_undo);
        break;
    case HistoryActionType::LinkAdd:
        pop_link_add(wf, step, do_undo);
        break;
    case HistoryActionType::LinkDel:
        pop_link_del(wf, step, do_undo);
        break;
    default:
        { LogAndThrowInternalError(ExcepCategory::Logic, "Unhandled undo/redo action"); }
    }
}

void HistoryManager::pop_node_add(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    if (!do_undo)
    {
        pop_node_del(dest_wf, step, true);
        return;
    }

    auto node = dest_wf->get_node_by_instance(step->id);
    if (nullptr == node) { LogAndThrowInternalError(ExcepCategory::Logic, "Node not found in dest wf"); }

    dest_wf->action()->delete_node(node);
}

void HistoryManager::pop_node_del(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    if (!do_undo)
    {
        pop_node_add(dest_wf, step, true);
        return;
    }

    const ActNodeBlob* real_step = cast::dynamic<const ActNodeBlob*>(step);
    blob::Reader reader(real_step->node);
    dest_wf->add_node(reader);
}

void HistoryManager::pop_node_clr(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodeClr* real_step = cast::dynamic<const ActNodeClr*>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    if (nullptr == node) { LogAndThrowInternalError(ExcepCategory::Logic, "Node not found in wf"); }

    dest_wf->action()->set_node_color(node, *std::get_if<ImU32>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_node_ren(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodeRen* real_step = cast::dynamic<const ActNodeRen*>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    if (nullptr == node) { LogAndThrowInternalError(ExcepCategory::Logic, "Node not found in wf"); }

    dest_wf->action()->set_node_name(node, *std::get_if<std::string>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_node_mode(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodeMode* real_step = cast::dynamic<const ActNodeMode*>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    if (nullptr == node) { LogAndThrowInternalError(ExcepCategory::Logic, "Node not found in wf"); } 

    dest_wf->action()->set_node_mode(node, *std::get_if<NodeMode>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_node_pin(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodePin* real_step = cast::dynamic<const ActNodePin*>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    if (nullptr == node) { LogAndThrowInternalError(ExcepCategory::Logic, "Node not found in wf"); } 

    dest_wf->action()->set_node_pinning(node, *std::get_if<bool>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_node_rect(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodeRect* real_step = cast::dynamic<const ActNodeRect*>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    if (nullptr == node) { LogAndThrowInternalError(ExcepCategory::Logic, "Node not found in wf"); } 

    dest_wf->action()->set_group_rect(node, *std::get_if<ImRect>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_pin_ren(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActPinRen* real_step = cast::dynamic<const ActPinRen*>(step);
    auto pin = dest_wf->get_pin_by_instance(step->id);
    if (nullptr == pin) { LogAndThrowInternalError(ExcepCategory::Logic, "Pin not found in wf"); } 

    dest_wf->action()->set_pin_name(pin, *std::get_if<std::string>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_pin_val(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActPinVal* real_step = cast::dynamic<const ActPinVal*>(step);
    auto pin = dest_wf->get_pin_by_instance(step->id);
    if (nullptr == pin) { LogAndThrowInternalError(ExcepCategory::Logic, "Pin not found in wf"); } 

    pin->state().value = *std::get_if<primitive>(do_undo ? &real_step->old_val : &real_step->new_val);
}

void HistoryManager::pop_link_add(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    if (!do_undo)
    {
        pop_link_del(dest_wf, step, true);
        return;
    }
    const ActLink* real_step = cast::dynamic<const ActLink*>(step);
    auto p1 = dest_wf->get_pin_by_instance(real_step->in);
    auto p2 = dest_wf->get_pin_by_instance(real_step->out);
    if (nullptr == p1 || nullptr == p2) { LogAndThrowInternalError(ExcepCategory::Logic, "Pin(s) not found in wf"); }
    dest_wf->action()->unlink(p1, p2);
}

void HistoryManager::pop_link_del(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    if (!do_undo)
    {
        pop_link_add(dest_wf, step, true);
        return;
    }

    const ActLink* real_step = cast::dynamic<const ActLink*>(step);
    auto p1 = dest_wf->get_pin_by_instance(real_step->in);
    auto p2 = dest_wf->get_pin_by_instance(real_step->out);
    if (nullptr == p1 || nullptr == p2) { LogAndThrowInternalError(ExcepCategory::Logic, "Pin(s) not found in wf"); }
    dest_wf->action()->link(p1, p2, real_step->id);
}

} // namespace mngr

