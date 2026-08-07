
#include "core/managers/history_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/serializer/wf_serializers.h"
#include "core/data/node.h"
#include "core/data/pin.h"

namespace mngr
{

/////////////////////////////////////////////////////

HistoryStep::HistoryStep(HistoryActionType t) noexcept
    : type(t)
{}
HistoryStep::~HistoryStep() noexcept = default;

HistoryAction::HistoryAction(int64_t id_, wid wf_) noexcept
    : id(id_)
    , wf(wf_)
{}
HistoryAction::~HistoryAction() noexcept = default;
HistoryAction::HistoryAction(HistoryAction&& other) noexcept
{
    *this = std::move(other);
}
HistoryAction& HistoryAction::operator=(HistoryAction&& other) noexcept
{
    if (this == &other)
        return *this;
    id = other.id;
    wf = other.wf;
    name = std::move(other.name);
    counter = other.counter;
    steps = std::move(other.steps);
    return *this;
}

////////////////////////////////////////////////////

HistoryManager::HistoryManager() noexcept
    : m_limit(200)
{}

HistoryManager::~HistoryManager() noexcept = default;

/////   interface
//////////////////////////////////////

void HistoryManager::cleanup(wid wf_id)
{
    m_meta.erase(wf_id);
    m_history.erase(wf_id);
    Log("History cleaned up for wf " << wf_id);
}

std::pair<bool, bool> HistoryManager::get_undo_redo(
    wid wf_id,
    const HistoryAction** undo_h, const HistoryAction** redo_h,
    const Meta** meta) const
{
    auto meta_it = m_meta.find(wf_id);
    if (meta_it == m_meta.end())
        return { false, false };

    auto h = m_history.find(wf_id);
    if (h == m_history.end()) return { false, false };

    bool undo_exists = false, redo_exists = false;

    if (meta != nullptr)
        *meta = &meta_it->second;

    if (meta_it->second.pos >= 0) // undo
    {
        undo_exists = true;
        auto& undo = h->second[meta_it->second.pos];
        if (undo_h != nullptr)
            *undo_h = &undo;
    }
    if (!h->second.empty() && meta_it->second.pos < cast::to_i32(h->second.size()) - 1)
    {
        redo_exists = true;
        auto& redo = h->second[meta_it->second.pos + 1];
        if (redo_h != nullptr)
            *redo_h = &redo;
    }

    return { undo_exists, redo_exists };
}

const HistoryAction* HistoryManager::get_undo(wid wf_id, const Meta** meta) const
{
    const HistoryAction* h = nullptr;
    const auto res = get_undo_redo(wf_id, &h, nullptr, meta);
    if (res.first)
        return h;
    return nullptr;
}

const HistoryAction* HistoryManager::get_redo(wid wf_id, const Meta** meta) const
{
    const HistoryAction* h = nullptr;
    const auto res = get_undo_redo(wf_id, nullptr, &h, meta);
    if (res.second)
        return h;
    return nullptr;
}

void HistoryManager::undo(wid wf_id)
{
    LogAssert(!m_rollback, "Rollback already in progress");
    m_rollback = true;

    const Meta* meta = nullptr;
    auto h = get_undo(wf_id, &meta);
    LogRAssert(h, "Can't undo, zero history ptr", );

    pop_action(h, true);
    (*cast::remove_const<Meta>(meta)).pos--;
    sync_wf_state(wf_id, meta);
    m_rollback = false;

    LogAction(*h, "DONE", meta->pos, meta->saved, m_history[h->wf].size());
}

void HistoryManager::redo(wid wf_id)
{
    LogAssert(!m_rollback, "Rollback already in progress");
    m_rollback = true;

    const Meta* meta = nullptr;
    auto h = get_redo(wf_id, &meta);
    LogRAssert(h, "Can't redo, zero history ptr", );

    pop_action(h, false);
    (*cast::remove_const<Meta>(meta)).pos++;
    sync_wf_state(wf_id, meta);
    m_rollback = false;

    LogAction(*h, "DONE", meta->pos, meta->saved, m_history[h->wf].size());
}

void HistoryManager::sync_wf_state(wid wf_id, const Meta* meta) const
{
    auto wf = g::pWfsManager->access(wf_id);
    auto& data = wf->get_data();

    const bool stable_state = (meta->pos == meta->saved && meta->saved >= -1); 
    data.dirty = !stable_state;
    data.draft = !stable_state;
}

bool HistoryManager::should_ignore_action(wid wf_id)
{
    if (m_rollback)
        return true;
    return m_ignore_actions.contains(wf_id);
}

void HistoryManager::set_ignore_actions_on(wid wf_id)
{
    m_ignore_actions.insert(wf_id);
}

void HistoryManager::set_ignore_actions_off(wid wf_id) noexcept
{
    m_ignore_actions.erase(wf_id);
}

void HistoryManager::set_rollback_pos(wid wf_id)
{
    auto& meta = m_meta[wf_id];
    meta.saved = meta.pos;
    Log("Wf " << wf_id << " rollback position set to " << meta.pos);
}

bool HistoryManager::can_rollback(wid wf_id)
{
    auto it = m_meta.find(wf_id);
    if (it == m_meta.end()) 
        return false;
    return it->second.pos != it->second.saved && it->second.saved >= -1;
}

void HistoryManager::rollback(wid wf_id)
{
    LogAssert(!m_rollback, "Rollback already in progress");
    LogRAssert(can_rollback(wf_id), "Can't rollback", );
    m_rollback = true;

    auto& meta = m_meta[wf_id];
    auto& h = m_history[wf_id];

    LogAction(wf_id, "ROLLBACK", meta.pos, meta.saved, h.size());

    while (meta.pos > meta.saved) // undo to saved pos
    {
        pop_action(&h[meta.pos], true);
        meta.pos--;
    }
    while (meta.pos < meta.saved) // redo to saved pos
    {
        meta.pos++;
        pop_action(&h[meta.pos], false);
    }

    sync_wf_state(wf_id, &meta);
    m_rollback = false;

    LogAction(wf_id, "ROLLBACK DONE", meta.pos, meta.saved, h.size());
}

HistoryState HistoryManager::get_state(wid wf_id) const
{
    const HistoryAction* undo_h = nullptr;
    const HistoryAction* redo_h = nullptr;
    const Meta* meta = nullptr;
    const auto res = get_undo_redo(wf_id, &undo_h, &redo_h, &meta);
    if (!res.first && !res.second)
        return {};

    HistoryState state;
    state.pos = meta->pos;
    state.saved_pos = meta->saved;
    if (undo_h != nullptr)
        state.undo_name = &undo_h->name;
    if (redo_h != nullptr)
        state.redo_name = &redo_h->name;

    return state;
}

/////   save action
//////////////////////////////////////

HistoryAction* HistoryManager::open_action(wid wf_id, const char* action_name)
{
    hqueue& h = m_history[wf_id];

    if (!h.empty() && h.back().counter > 0)
    {
        h.back().counter++;
        LogAction(h.back(), "Continuing");
        return &h.back();
    }

    HistoryAction act(++m_last_id, wf_id);
    act.name = action_name;
    act.counter = 1;
    h.push_back(std::move(act));

    LogAction(h.back(), "Opened");
    return &h.back();
}

void HistoryManager::close_action(wid wf_id)
{
    hqueue& h = m_history[wf_id];
    LogRAssert(!h.empty(), "No actions to close", );
    
    auto& act = h.back();
    LogRAssert(act.counter != 0, "Actions already closed", );

    if (--act.counter > 0)
    {
        LogAction(act, "Pending Close");
        return;
    }

    if (act.steps.empty())
    {
        LogAction(act, "Dropped (Empty)");
        h.pop_back();
        return;
    }

    auto& meta = m_meta[wf_id];
    HistoryAction valid_act = std::move(h.back());
    h.pop_back();

    const size_t sz = h.size();
    while (cast::to_i32(h.size()) - 1 > meta.pos)
    {
        h.pop_back();
    }
    if (meta.saved > meta.pos)
    {
        meta.saved = -2;
        Log("Wf " << wf_id << " saved pos dropped, rollback only via DB");
    }
    if (sz != h.size())
    {
        Log("Wf " << wf_id << ", dropped " << sz - h.size() << " actions on back");
    }

    h.push_back(std::move(valid_act));
    meta.pos++;

    sync_wf_state(wf_id, &meta);
    
    const size_t sz_before_limit = h.size();
    while (h.size() > m_limit)
    {
        h.pop_front();
        if (meta.pos > -1) meta.pos--;
        meta.saved--;
    }
    if (sz_before_limit != h.size())
    {
        Log("Wf " << wf_id << ", dropped " << sz_before_limit - h.size() << " actions on front");
    }
    LogAction(h.back(), "Closed", meta.pos, meta.saved, h.size());
}

void HistoryManager::multi_action_begin(wid wf_id, const char* action_name)
{
    if (should_ignore_action(wf_id)) return;
    open_action(wf_id, action_name);
}

void HistoryManager::multi_action_end(wid wf_id)
{
    if (should_ignore_action(wf_id)) return;
    close_action(wf_id);
}

void HistoryManager::push_node_action(const data::NodeInstance* node, HistoryActionType act)
{
    LogRAssert(node, "Zero ptr", );
    if (should_ignore_action(node->get_workflow_id())) return;

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
        LogRError("Invalid action pushed", );
    }

    LogRAssert(history->wf == node->get_workflow_id(), "Wfs don't match. hw:" << history->wf << " != nw:" << node->get_workflow_id() << ", n:" << node->get_instance_id(), );

    step->id = node->get_instance_id();
    step->debug_name = node->get_name();
    step->node = data::WfSerializerToBlob(nullptr).serialize_node(node);
    history->steps.push_back(std::move(step));

    close_action(node->get_workflow_id());
}

void HistoryManager::push_node_action(const data::NodeInstance* node, HistoryActionType act, HActionT&& old_val)
{
    LogRAssert(node, "Zero ptr", );
    if (should_ignore_action(node->get_workflow_id())) return;

    HistoryAction* history = nullptr;
    std::unique_ptr<ActOldNew> step;

    switch (act)
    {
    case HistoryActionType::NodeClr:
        history = open_action(node->get_workflow_id(), "Set color");
        step = std::make_unique<ActNodeClr>();
        step->new_val = (node->m_group ? node->m_clr_group : ImGui::ColorConvertFloat4ToU32(node->m_clr_node));
        break;
    case HistoryActionType::NodeExpand:
        history = open_action(node->get_workflow_id(), node->get_expanded() ? "Expand" : "Collapse");
        step = std::make_unique<ActNodeExpand>();
        step->new_val = node->get_expanded();
        break;
    case HistoryActionType::NodeMode:
        history = open_action(node->get_workflow_id(), "Set mode");
        step = std::make_unique<ActNodeMode>();
        step->new_val = node->get_mode();
        break;
    case HistoryActionType::NodePin:
        history = open_action(node->get_workflow_id(), "Pin node");
        step = std::make_unique<ActNodePin>();
        step->new_val = node->get_pinned();
        break;
    case HistoryActionType::NodeRect:
        history = open_action(node->get_workflow_id(), "Move");
        step = std::make_unique<ActNodeRect>();
        step->new_val = node->m_rc;
        break;
    case HistoryActionType::NodeRen:
        history = open_action(node->get_workflow_id(), "Rename");
        step = std::make_unique<ActNodeRen>();
        step->new_val = node->get_name();
        break;
    default:
        LogRError("Invalid action pushed", );
    }

    LogRAssert(history->wf == node->get_workflow_id(), "Wfs don't match. hw:" << history->wf << " != nw:" << node->get_workflow_id() << ", n:" << node->get_instance_id(), );

    step->id = node->get_instance_id();
    step->old_val = std::move(old_val);
    history->steps.push_back(std::move(step));

    close_action(node->get_workflow_id());
}

void HistoryManager::push_pin_action(const data::PinInstance* pin, HistoryActionType act, HActionT&& old_val)
{
    LogRAssert(pin, "Zero ptr", );
    if (should_ignore_action(pin->get_workflow_id())) return;

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
    {
        LogRError("Invalid action pushed", );
    }

    LogRAssert(history->wf == pin->get_workflow_id(), "Wfs don't match. hw:" << history->wf << " != pw:" << pin->get_workflow_id() << ", p:" << pin->get_instance_id(), );
    
    step->id = pin->get_instance_id();
    step->old_val = std::move(old_val);
    history->steps.push_back(std::move(step));

    close_action(pin->get_workflow_id());
}

void HistoryManager::push_link_action(const data::Link* link, HistoryActionType act)
{
    LogRAssert(link, "Zero ptr", );
    if (should_ignore_action(link->get_workflow_id())) return;

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
    {
        LogRError("Invalid action pushed", );
    }

    LogRAssert(history->wf == link->get_workflow_id(), "Wfs don't match. hw:" << history->wf << " != lw:" << link->get_workflow_id() << ", l:" << link->get_instance_id(), );

    step->id = link->get_instance_id();
    step->node_in = link->get_input()->get_parent_node();
    step->pin_in = link->get_input()->get_instance_id();
    step->pin_in_name = link->get_input()->get_object()->def_state.name;
    step->node_out = link->get_output()->get_parent_node();
    step->pin_out = link->get_output()->get_instance_id();
    step->pin_out_index = link->get_output()->get_object()->out_index;
    step->linked_pins_type = link->get_input()->get_object()->def_state.type_str;
    history->steps.push_back(std::move(step));

    close_action(link->get_workflow_id());
}

////////  undo / redo
//////////////////////////////////////

void HistoryManager::pop_action(const HistoryAction* action, bool do_undo)
{
    LogRAssert(action, "Zero ptr", );
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
}

void HistoryManager::pop_action(data::Workflow* wf, const HistoryStep* step, bool do_undo)
{
    LogRAssert(wf && step, "Zero ptr", );

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
    case HistoryActionType::NodeExpand:
        pop_node_expand(wf, step, do_undo);
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
        LogRError("Unhandled undo/redo action", );
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
    LogRAssert(node != nullptr, "Node not found in wf. w:" << dest_wf->get_id() << ", n:" << step->id, );

    // we have to re-save node cuz if could be changed
    const ActNodeBlob* real_step = cast::dynamic<const ActNodeBlob>(step);
    cast::remove_const<ActNodeBlob>(real_step)->node = data::WfSerializerToBlob(nullptr).serialize_node(node);

    dest_wf->action()->delete_node(node);
}

void HistoryManager::pop_node_del(data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    if (!do_undo)
    {
        pop_node_add(dest_wf, step, true);
        return;
    }

    const ActNodeBlob* real_step = cast::dynamic<const ActNodeBlob>(step);
    blob::Reader reader(real_step->node);
    try
    {
        data::WfDeserializerFromBlob(dest_wf).deserialize_node(reader);
    }
    catch (const std::exception&)
    {
        LogExcep("Node deserialization failed");
    }
}

void HistoryManager::pop_node_clr(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodeClr* real_step = cast::dynamic<const ActNodeClr>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    LogRAssert(node != nullptr, "Node not found in wf. w:" << dest_wf->get_id() << ", n:" << step->id, );

    dest_wf->action()->set_node_color(node, *std::get_if<ImU32>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_node_expand(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodeExpand* real_step = cast::dynamic<const ActNodeExpand>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    LogRAssert(node != nullptr, "Node not found in wf. w:" << dest_wf->get_id() << ", n:" << step->id, );

    dest_wf->action()->set_node_expanded(node, *std::get_if<bool>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_node_ren(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodeRen* real_step = cast::dynamic<const ActNodeRen>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    LogRAssert(node != nullptr, "Node not found in wf. w:" << dest_wf->get_id() << ", n:" << step->id, );

    dest_wf->action()->set_node_name(node, *std::get_if<std::string>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_node_mode(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodeMode* real_step = cast::dynamic<const ActNodeMode>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    LogRAssert(node != nullptr, "Node not found in wf. w:" << dest_wf->get_id() << ", n:" << step->id, );

    dest_wf->action()->set_node_mode(node, *std::get_if<NodeMode>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_node_pin(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodePin* real_step = cast::dynamic<const ActNodePin>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    LogRAssert(node != nullptr, "Node not found in wf. w:" << dest_wf->get_id() << ", n:" << step->id, );

    dest_wf->action()->set_node_pinning(node, *std::get_if<bool>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_node_rect(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActNodeRect* real_step = cast::dynamic<const ActNodeRect>(step);
    auto node = dest_wf->get_node_by_instance(step->id);
    LogRAssert(node != nullptr, "Node not found in wf. w:" << dest_wf->get_id() << ", n:" << step->id, );

    dest_wf->action()->set_node_rect(node, *std::get_if<ImRect>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_pin_ren(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActPinRen* real_step = cast::dynamic<const ActPinRen>(step);
    auto pin = dest_wf->get_pin_by_instance(step->id);
    LogRAssert(pin != nullptr, "Node not found in wf. w:" << dest_wf->get_id() << ", n:" << step->id, );

    dest_wf->action()->set_pin_name(pin, *std::get_if<std::string>(do_undo ? &real_step->old_val : &real_step->new_val));
}

void HistoryManager::pop_pin_val(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    const ActPinVal* real_step = cast::dynamic<const ActPinVal>(step);
    auto pin = dest_wf->get_pin_by_instance(step->id);
    LogRAssert(pin != nullptr, "Node not found in wf. w:" << dest_wf->get_id() << ", n:" << step->id, );

    pin->state().value = *std::get_if<Primitive>(do_undo ? &real_step->old_val : &real_step->new_val);
}

void HistoryManager::pop_link_add(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    if (!do_undo)
    {
        pop_link_del(dest_wf, step, true);
        return;
    }
    const ActLink* real_step = cast::dynamic<const ActLink>(step);
    const auto* p1 = dest_wf->get_pin_by_instance(real_step->pin_in);
    const auto* p2 = dest_wf->get_pin_by_instance(real_step->pin_out);
    LogRAssert(p1 != nullptr && p2 != nullptr, "Pin(s) not found in wf. w:" << dest_wf->get_id() << ", p1:" << real_step->pin_in << ":" << (p1 != nullptr) << ", p2:" << real_step->pin_out << ":" << (p2 != nullptr), );
    
    dest_wf->action()->unlink(p1, p2);
}

void HistoryManager::pop_link_del(const data::Workflow* dest_wf, const HistoryStep* step, bool do_undo)
{
    if (!do_undo)
    {
        pop_link_add(dest_wf, step, true);
        return;
    }

    const ActLink* real_step = cast::dynamic<const ActLink>(step);
    auto p1 = dest_wf->get_pin_by_instance(real_step->pin_in);
    auto p2 = dest_wf->get_pin_by_instance(real_step->pin_out);
    LogRAssert(p1 != nullptr && p2 != nullptr, "Pin(s) not found in wf. w:" << dest_wf->get_id()
        << ", n1:" << real_step->node_in << ", p1:" << real_step->pin_in << ":" << (p1 != nullptr)
        << ", n2:" << real_step->node_out << ", p2:" << real_step->pin_out << ":" << (p2 != nullptr), );

    dest_wf->action()->link(p1, p2, real_step->id);
}

} // namespace mngr

