
#include "core/managers/workflows_manager.h"
#include "core/managers/db_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/history_manager.h"
#include "core/workflow/link.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/workflow_serializer_to_blob.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

WfsManager::WfsManager() = default;
WfsManager::~WfsManager() = default;

void WfsManager::stop_all_threads()
{
    LogF;
    if (g::pGraphN != nullptr)
        g::pGraphN->unsubscribe_timers(this);
}

bool WfsManager::init()
{
    LogF;
    if (!Manager::init())
        return false;

    auto wf_datas = g::pDbManager->wf_load_meta_and_purge();
    for (auto& data : wf_datas)
    {
        if (data.name.find("Workflow ") == 0)
        {
            const auto num = cast::to_u32(std::strtoul(data.name.c_str() + strlen("Workflow "), nullptr, 10));
            m_wf_name_counter = std::max(m_wf_name_counter, num);
        }

        create(std::move(data), false);
    }

    init_labels();

    g::pGraphN->subscribe_timers(this);
    return true;
}

void WfsManager::free()
{
    LogF;
    m_active = 0;
    m_wfs.clear();
    m_wfs_ro.clear();
    m_labels.clear();
}

const WfData& WfsManager::create(bool generate_name, bool dump_to_db, bool ro)
{
    WfData data;
    data.ro = ro;
    if (generate_name)
        data.name = "Workflow " + std::to_string(++m_wf_name_counter);
    auto wf = create(std::move(data), dump_to_db);
    return wf->get_data();
}

const WfData& WfsManager::create_copy(wid source)
{
    auto src_wf = access(source);
    
    WfData data;
    data.name = src_wf->get_data().name + " (copy)";
    auto wf = create(std::move(data), true);

    copy_begin(source, ImVec2(0, 0));
    auto& nodes = src_wf->get_nodes();
    for (auto& node : nodes)
    {
        copy_add(node.get());
    }
    copy_end(wf->get_id(), ImVec2(0, 0));
    copy_begin(0, {});

    return wf->get_data();
}

void WfsManager::load(wid wf_id, blob::Blob& drawer_state)
{
    auto wf = access(wf_id);
    auto& data = wf->get_data();
    if (!data.draft_exist && !data.saved_exist || data.ro)
        return;

    blob::Blob blob;
    bool load_saved = !data.draft_exist;
    g::pDbManager->wf_load_blob(data.uuid, blob, load_saved);
    const bool result = data::WorkflowSerializerToBlob::deserialize(wf, blob, drawer_state);
    data.draft = !load_saved || !result;
}

data::Workflow* WfsManager::create(WfData&& data, bool dump_to_db)
{
    const bool ro = data.ro;
    auto wf = std::make_unique<data::Workflow>(std::move(data));
    auto wf_ptr = wf.get();
    wf_ptr->init();
    auto& new_data = wf_ptr->get_data();
    if (ro)
        m_wfs_ro.push_back(std::move(wf));
    else
        m_wfs[new_data.id] = std::move(wf);
    if (dump_to_db)
        g::pDbManager->wf_update_meta(new_data);
    return wf_ptr;
}

data::Workflow* WfsManager::access(wid id) const
{
    const auto search_rw = [this, id]() -> data::Workflow*
    {
        const auto it = m_wfs.find(id);
        if (it == m_wfs.end())
            return nullptr;
        return it->second.get();
    };

    const auto search_ro = [this, id]() -> data::Workflow*
    {
        const auto it = std::find_if(m_wfs_ro.begin(), m_wfs_ro.end(), [id](const std::unique_ptr<data::Workflow>& ptr) { return ptr->get_id() == id; });
        if (it == m_wfs_ro.end())
            return nullptr;
        return (*it).get();
    };

    auto wf = search_rw();
    if (nullptr == wf)
        wf = search_ro();
    if (nullptr == wf)
    {
        LogInternalLogic("Wf id=" << id << " not found");
        return nullptr;
    }
    return wf;
}

void WfsManager::set_active(wid id)
{
    if (m_active == id)
        return;
    if (m_active != 0)
        dump(m_active);

    if (0 == id)
    {
        g::pOptsManager->wf_active_save("");
        m_active = 0;
    }
    else
    {
        auto& data = access(id)->get_data();
        g::pOptsManager->wf_active_save(data.uuid);
        m_active = data.id;
    }    
    Log("Wf id=" << id << " activated");
}

void WfsManager::update_name(wid id, const std::string& name)
{
    auto& data = get_data(id);
    data.name = name;
    g::pDbManager->wf_update_meta(data);
    Log("Wf id=" << id << " n=" << name);
}

namespace
{
std::vector<WfData*> filter_wfs(const std::unordered_map<wid, std::unique_ptr<data::Workflow>>& wfs, const std::function<bool(const WfData&)>& filter)
{
    std::vector<WfData*> ret;
    for (auto& [id, wf] : wfs)
    {
        auto& data = wf->get_data();
        if (filter(data))
            ret.push_back(&data);
    }
    return ret;
}
}

std::vector<WfData*> WfsManager::get_wfs() const
{
    return filter_wfs(m_wfs, [](const WfData&) { return true; });
}

std::vector<WfData*> WfsManager::get_undeleted() const
{
    return filter_wfs(m_wfs, [](const WfData& data) { return !data.pending_delete; });
}

std::vector<WfData*> WfsManager::get_deleted() const
{
    return filter_wfs(m_wfs, [](const WfData& data) { return data.pending_delete; });
}

void WfsManager::delete_wf(wid id)
{
    auto& data = get_data(id);
    data.pending_delete = true;
    g::pDbManager->wf_update_meta(data);
    set_wf_labels(id, {});
    Log("Wf id=" << id << " deleted");
}

void WfsManager::undelete_wf(wid id)
{
    auto& data = get_data(id);
    data.pending_delete = false;
    g::pDbManager->wf_update_meta(data);
    Log("Wf id=" << id << " restored");
}

bool WfsManager::is_deleted(wid id) const
{
    auto& data = get_data(id);
    return data.pending_delete;
}

void WfsManager::init_labels()
{
    m_labels.clear();
    m_wfs_in_label.clear();
    m_last_label_id = 0;

    auto labels = g::pOptsManager->wf_labels_load(tools::str::hash_fnv1a(DlgObjID[cast::to_szt(DlgObj::PanelWfList)]));
    if (labels.empty())
        return;

    Log("Wfs labels loaded: " << labels);
    try
    {
        using json = nlohmann::json;
        const json j = json::parse(labels);

        for (auto& obj : j)
        {
            std::string name = obj["name"];
            uint32_t id = obj["id"];
            m_labels.push_back({ name, id });

            if (id > m_last_label_id)
                m_last_label_id = id;

            auto& wfs_in_label = m_wfs_in_label[id];
            std::vector<wid> wf_ids = obj["wfs"];
            for (auto wf_id : wf_ids)
            {
                for (auto& wf : m_wfs)
                {
                    if (wf.second->get_id() == wf_id)
                    {
                        wfs_in_label.push_back(wf_id);
                        break;
                    }
                }
            }

            sort_wfs(wfs_in_label);
        }

        m_labels.sort([](const std::pair<std::string, uint32_t>& l, const std::pair<std::string, uint32_t>& r)
            { return tools::str::to_lower_ascii(l.first) < tools::str::to_lower_ascii(r.first); });
    }
    catch (const std::exception& ex)
    {
        LogInternalLogic("Failed to parse labels (" << labels << "). Err: " << ex.what());
    }
}

uint32_t WfsManager::create_label()
{
    std::string label = "new label";
    size_t label_idx = 1;
    for (auto it = m_labels.begin(); it != m_labels.end(); )
    {
        const auto& name = it->first;
        if (tools::str::to_lower_ascii(name) == label)
        {
            label = fmt::format(FMT_STRING("new label ({})"), ++label_idx);
            it = m_labels.begin();
            continue;
        }
        ++it;
    }

    m_last_label_id++;
    m_wfs_in_label[m_last_label_id];

    Log("New WF label added: " << label << " = " << m_last_label_id);

    m_labels.push_back({ label, m_last_label_id });
    m_labels.sort([](const std::pair<std::string, uint32_t>& l, const std::pair<std::string, uint32_t>& r)
        { return tools::str::to_lower_ascii(l.first) < tools::str::to_lower_ascii(r.first); });

    save_labels();
    return m_last_label_id;
}

bool WfsManager::rename_label(uint32_t label_id, const std::string& new_name)
{
    std::list<std::pair<std::string, uint32_t>>::iterator label_it = m_labels.begin();
    for (auto it = m_labels.begin(); it != m_labels.end(); ++it)
    {
        if (it->second == label_id)
        {
            if (it->first == new_name)
                return false;
            label_it = it;
        }
        else if (it->first == new_name)
            return false;
    }

    if (label_it == m_labels.end())
    {
        LogInternalLogic("Can't rename label " << label_id << ": id not found");
        return false;
    }

    Log("Label renamed from '" << label_it->first << "' to '" << new_name << "'");
    label_it->first = new_name;
    m_labels.sort([](const std::pair<std::string, uint32_t>& l, const std::pair<std::string, uint32_t>& r)
        { return tools::str::to_lower_ascii(l.first) < tools::str::to_lower_ascii(r.first); });

    save_labels();
    return true;
}

void WfsManager::remove_label(uint32_t label_id)
{
    bool label_found = false;
    for (auto it = m_labels.begin(); it != m_labels.end(); ++it)
    {
        if (it->second == label_id)
        {
            Log("Label " << it->first << " renamed");
            m_labels.erase(it);
            label_found = true;
            break;
        }
    }

    if (!label_found)
    {
        LogInternalLogic("Can't remove label " << label_id << ": id not found");
        return;
    }

    m_wfs_in_label.erase(label_id);
    save_labels();
}

void WfsManager::set_wf_labels(wid wf_id, std::set<uint32_t> labels)
{
    bool changed = false;
    for (auto& [label_id, wf_ids] : m_wfs_in_label)
    {
        const auto it = std::find(wf_ids.begin(), wf_ids.end(), wf_id);
        if (it == wf_ids.end() && labels.count(label_id) > 0)
        {
            wf_ids.push_back(wf_id);
            sort_wfs(wf_ids);
            changed = true;
        }
        else if (it != wf_ids.end() && labels.count(label_id) == 0)
        {
            wf_ids.erase(it);
            changed = true;
        }
    }

    if (changed)
        save_labels();
}

void WfsManager::save_labels() const
{
    nlohmann::json j;
    for (auto& [label, id] : m_labels)
    {        
        nlohmann::json jlabel;
        jlabel["name"] = label;
        jlabel["id"] = id;

        auto it = m_wfs_in_label.find(id);
        if (it != m_wfs_in_label.end())
        {
            std::vector<wid> wf_ids;
            wf_ids.reserve(it->second.size());
            for (auto wf_id : it->second)
            {
                wf_ids.push_back(wf_id);
            }
            jlabel["wfs"] = std::move(wf_ids);
        }
        else
        {
            jlabel["wfs"] = std::vector<wid>{};
        }

        j.push_back(std::move(jlabel));
    }

    g::pOptsManager->wf_labels_save(tools::str::hash_fnv1a(DlgObjID[cast::to_szt(DlgObj::PanelWfList)]), j.dump());
}

const std::list<wid>& WfsManager::get_wfs_in_label(uint32_t label_id) const
{
    auto it = m_wfs_in_label.find(label_id);
    if (it == m_wfs_in_label.end())
    {
        static std::list<wid> empty;
        return empty;
    }
    return it->second;
}

bool WfsManager::get_is_wf_in_label(wid wf_id, uint32_t label_id) const
{
    auto& ids = get_wfs_in_label(label_id);
    return std::find(ids.begin(), ids.end(), wf_id) != ids.end();
}

void WfsManager::sort_wfs(std::list<wid>& wfs)
{
    wfs.sort([this](wid l_id, wid r_id)
    {
        auto l = access(l_id);
        auto r = access(r_id);
        return tools::str::to_lower_ascii(l->get_data().name) < tools::str::to_lower_ascii(r->get_data().name);
    });
}

void WfsManager::copy_begin(wid source, std::optional<ImVec2> src_point)
{
    if (0 == source)
    {
        Log("Copy reset");
        m_copy_src = 0;
        m_copied_nodes.clear();
        return;
    }
    m_copy_src = source;
    m_copy_point = src_point;
    m_copied_nodes.clear();

    if (m_copy_point.has_value())
    {
        Log("Initiated copy from " << source << " at " << m_copy_point.value().x << "/" << m_copy_point.value().y);
    }
    else
    {
        Log("Initiated copy from " << source << " at undefined point");
    }
}

void WfsManager::copy_add(data::NodeInstance* node)
{
    if (0 == m_copy_src)
    {
        LogInternalLogic("Copy wasn't initiated");
        return;
    }
    if (m_copy_src != node->get_workflow_id())
    {
        LogInternalLogic("Node to copy is from diff wf");
        return;
    }
    m_copied_nodes.insert(node);
    Log("Node " << node->get_instance_id() << " added to copy");
}

void WfsManager::copy_del(data::NodeInstance* node)
{
    if (0 == m_copy_src)
        return;
    if (m_copied_nodes.erase(node) > 0)
    {
        Log("Node " << node->get_instance_id() << " removed from copy");
    }
}

void WfsManager::copy_end(wid dest, std::optional<ImVec2> dest_point)
{
    if (0 == m_copy_src)
    {
        LogInternalLogic("Copy wasn't initiated");
        return;
    }
    if (0 == dest)
    {
        LogInternalLogic("Dest wf is uindefined");
        return;
    }
    if (m_copied_nodes.empty())
    {
        Log("No nodes to copy");
        return;
    }

    ImVec2 src_pt(FLT_MAX, FLT_MAX);
    if (!m_copy_point.has_value())
    {
        for (auto node : m_copied_nodes)
        {
            src_pt.x = std::min(src_pt.x, node->m_rc.Min.x);
            src_pt.y = std::min(src_pt.y, node->m_rc.Min.y);
        }
        Log("Copy source point set to " << src_pt.x << "/" << src_pt.y);
    }
    else
        src_pt = m_copy_point.value();

    ImVec2 dest_pt;
    if (!dest_point.has_value())
    {
        dest_pt.x = dest_pt.y = 0;
        Log("Copy source point set to 0/0");
    }
    else
        dest_pt = dest_point.value();


    auto src_wf = access(m_copy_src);
    auto dest_wf = access(dest);

    g::pHistoryManager->multi_action_begin(dest, "Paste");

    std::map<iid, data::NodeInstance*> curr_node_to_new_node;

    for (auto node : m_copied_nodes)
    {
        auto new_node = dest_wf->add_node(node);
        new_node->m_rc.Min.x = node->m_rc.Min.x - src_pt.x;
        new_node->m_rc.Min.x = dest_pt.x + new_node->m_rc.Min.x;
        new_node->m_rc.Min.y = node->m_rc.Min.y - src_pt.y;
        new_node->m_rc.Min.y = dest_pt.y + new_node->m_rc.Min.y;
        new_node->m_rc.Max = node->m_rc.Max;

        g::pHistoryManager->push_node_action(new_node, HistoryActionType::NodeAdd);

        curr_node_to_new_node[node->get_instance_id()] = new_node;
    }

    const auto& links = src_wf->get_links();
    for (auto& link : links)
    {
        const auto src_in_pin = link->get_input();
        const auto src_out_pin = link->get_output();

        const auto in_it = curr_node_to_new_node.find(src_in_pin->get_parent_node());
        const auto out_it = curr_node_to_new_node.find(src_out_pin->get_parent_node());
        if (in_it == curr_node_to_new_node.end() || out_it == curr_node_to_new_node.end())
            continue;

        const data::NodeInstance* dest_in_node = in_it->second;
        const data::NodeInstance* dest_out_node = out_it->second;
        data::PinInstance* dest_in_pin = nullptr;
        data::PinInstance* dest_out_pin = nullptr;

        for (auto pin : dest_in_node->get_inputs())
        {
            if (pin->get_object()->get_id() == src_in_pin->get_object()->get_id())
            {
                dest_in_pin = pin;
                break;
            }
        }
        for (auto pin : dest_out_node->get_outputs())
        {
            if (pin->get_object()->get_id() == src_out_pin->get_object()->get_id())
            {
                dest_out_pin = pin;
                break;
            }
        }
        if (nullptr == dest_in_pin || nullptr == dest_out_pin)
        {
            LogInternalLogic("Dest pin not found");
        }
        else
        {
            auto new_link = data::Link::link(dest_in_pin, dest_out_pin);
            g::pHistoryManager->push_link_action(new_link, HistoryActionType::LinkAdd);
        }
    }

    g::pHistoryManager->multi_action_end(dest);

    g::pFlow->wf_rebuild(dest);
    Log("Copy " << m_copied_nodes.size() << " from " << m_copy_src << " to " << dest << " at " << dest_pt.x << "/" << dest_pt.y);
}

void WfsManager::close_and_purge()
{
    for (auto& [id, wf] : m_wfs)
    {
        auto& data = wf->get_data();

        if (!data.draft_exist && !data.saved_exist && wf->get_nodes().empty())
        {
            data.pending_delete = true;
            g::pDbManager->wf_update_meta(data);
            Log("Wf " << data.uuid << "###" << data.name << " (" << id << ") marked to pending delete");
        }
        else
            close(id);
    }
}

void WfsManager::close(wid wf_id)
{
    auto wf = access(wf_id);
    auto& data = wf->get_data();

    if (data.dirty)
        dump(wf_id);
}

void WfsManager::save(wid wf_id)
{
    auto wf = access(wf_id);
    auto& data = wf->get_data();
    if (!data.dirty && !data.draft || data.ro)
        return;

    g::pHistoryManager->set_rollback_pos(wf_id);
    data.draft = false;
    data.saved_exist = true;
    data.draft_exist = false;
    data.dirty = true;
    dump(wf_id);
}

void WfsManager::revert(wid wf_id, blob::Blob& drawer_state)
{
    auto wf = access(wf_id);
    auto& data = wf->get_data();

    const bool can_rollback = g::pHistoryManager->can_rollback(wf_id);
    if (!can_rollback && !data.saved_exist)
        return;

    if (can_rollback)
    {
        g::pHistoryManager->rollback(wf_id);
        Log("Wf " << wf_id << " reverted via history rollback");
    }
    else
    {
        blob::Blob blob;
        bool load_saved = true;
        g::pDbManager->wf_load_blob(data.uuid, blob, load_saved);
        if (!load_saved)
        {
            LogWarn("Wf " << wf_id << ": failed to load state");
            return;
        }

        const bool res = data::WorkflowSerializerToBlob::deserialize(wf, blob, drawer_state);
        g::pHistoryManager->cleanup(wf_id);
        Log("Wf " << wf_id << " reverted via hard reset, res: " << (res ? "ok" : "fail"));
    }

    data.dirty = false;
    data.draft = false;
    data.saved_exist = true;
}

void WfsManager::dump(wid wf_id) const
{
    auto wf = access(wf_id);
    auto& data = wf->get_data();
    if (!data.dirty || data.critical_failure || data.ro)
        return;

    blob::Blob empty;
    const blob::Blob* drawer_state = nullptr;
    auto wf_action = wf->action();
    if (wf_action != nullptr)
        drawer_state = wf_action->get_drawer_state();
    if (nullptr == drawer_state)
        drawer_state = &empty;

    auto wr = data::WorkflowSerializerToBlob::serialize(wf, *drawer_state);
    g::pDbManager->wf_update_blob(data.uuid, wr.release(), data.draft);

    data.dirty = false;
    if (data.draft)
    {
        data.draft_exist = true;
        data.draft_time = tools::dt::from_current_date();
    }
    else
    {
        data.saved_exist = true;
        data.saved_time = tools::dt::from_current_date();
    }
    
    Log("Wf " << wf_id << " dumped " << (data.draft ? "(draft)" : "(explicit save)"));
}

void WfsManager::on_1s_lazy_timer(int64_t /*total_seconds_elapsed*/)
{
    m_dump_counter++;
    if (m_dump_counter >= 60)
    {
        if (ImGui::IsAnyMouseDown())
        {
            Log("Canvas is busy, save later");
            return;
        }

        for (auto& wf : m_wfs)
        {
            dump(wf.first);
        }
        m_dump_counter = 0;
    }
}

} // namespace mngr
