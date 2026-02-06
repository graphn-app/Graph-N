
#include "core/managers/workflows_manager.h"
#include "core/managers/db_manager.h"
#include "core/managers/settings_manager.h"
#include "core/workflow/workflow.h"

namespace mngr
{

WfsManager::WfsManager() = default;
WfsManager::~WfsManager() = default;

void WfsManager::stop_all_threads()
{
    LogF;
}

bool WfsManager::init()
{
    LogF;

    auto& active = g::pOptsManager->wf_active_load();
    auto wf_datas = g::pDbManager->wf_load_meta_and_purge();
    std::set<std::string> labels;

    for (auto& wd : wf_datas)
    {
        if (!wd.labels_raw.empty())
        {
            auto lbs = tools::str::split(wd.labels_raw, '|');
            for (auto& lb : lbs)
            {
                wd.labels.insert(lb);
                labels.insert(lb);
            }
        }

        auto wf = create(std::move(wd));
        if (wf->get_uuid() == active)
            m_active = wf->get_id();
    }

    std::for_each(labels.begin(), labels.end(), [this, &lbs = m_labels](const std::string& label) { Log("Wf label: " << label); lbs.push_back(label); });
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

WfInfo WfsManager::from_data(const WfData& data) const
{
    WfInfo info;
    info.id = data.id;
    info.uuid = data.uuid;
    info.name = data.name;
    return info;
}

WfInfo WfsManager::create(bool skip_name)
{
    WfData data;
    if (!skip_name)
        data.name = "Workflow " + std::to_string(++m_wf_name_counter);
    auto wf = create(std::move(data));
    return from_data(wf->get_data());
}

data::Workflow* WfsManager::create(WfData&& data)
{
    auto wf = std::make_unique<data::Workflow>(std::move(data));
    auto wf_ptr = wf.get();
    wf_ptr->init();
    auto& new_data = wf_ptr->get_data();
    m_wfs[new_data.id] = std::move(wf);
    g::pDbManager->wf_update_meta(new_data);
    return wf_ptr;
}

data::Workflow* WfsManager::find(wid id) const
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

WfData& WfsManager::find_data(wid id) const
{
    return find(id)->get_data();
}

void WfsManager::set_active(wid id)
{
    if (0 == id)
    {
        g::pOptsManager->wf_active_save("");
        m_active = 0;
    }
    else
    {
        auto& data = find_data(id);
        g::pOptsManager->wf_active_save(data.uuid);
        m_active = data.id;
    }    
    Log("Wf id=" << id << " activated");
}

void WfsManager::set_read_only(wid id)
{
    auto it = m_wfs.find(id);
    if (it == m_wfs.end())
    {
        LogInternalLogic("Wf id=" << id << " not found");
        return;
    }
    auto ptr = std::move(it->second);
    m_wfs.erase(it);

    ptr->get_data().ro = true;
    m_wfs_ro.push_back(std::move(ptr));

    Log("Wf id=" << id << " set RO");
}

void WfsManager::update_name(wid id, const std::string& name)
{
    auto& data = find_data(id);
    data.name = name;
    g::pDbManager->wf_update_meta(data);
    Log("Wf id=" << id << " n=" << name);
}

void WfsManager::update_history(wid id, uint32_t history_pos, uint32_t history_steps)
{
    auto& data = find_data(id);
    data.history_steps = history_steps;
    data.history_pos = history_pos;
    g::pDbManager->wf_update_meta(data);
    Log("Wf id=" << id << " hp=" << history_pos << ", hs=" << history_steps);
}

void WfsManager::update_json(wid id, std::string&& json)
{
    Log("Wf id=" << id << " json upd sz=" << json.size());
    auto& data = find_data(id);
    std::swap(data.json, json);
    g::pDbManager->wf_update_json(data);
}

const std::string& WfsManager::get_json(wid id)
{
    auto& data = find_data(id);
    if (data.json.empty())
        g::pDbManager->wf_load_json(data);
    return data.json;
}

std::vector<WfInfo> WfsManager::get_infos(bool deleted) const
{
    std::vector<WfInfo> ret;
    ret.reserve(m_wfs.size());
    for (auto& wf : m_wfs)
    {
        auto& data = wf.second->get_data();
        if (data.deleted && deleted || !data.deleted && !deleted)
            ret.push_back(from_data(data));
    }
    return ret;
}

std::vector<WfInfo> WfsManager::get_wfs() const
{
    return get_infos(false);
}

std::vector<WfInfo> WfsManager::get_deleted() const
{
    return get_infos(true);
}

void WfsManager::mark_deleted(wid id)
{
    auto& data = find_data(id);
    data.deleted = true;
    g::pDbManager->wf_update_meta(data);
    Log("Wf id=" << id << " deleted");
}

void WfsManager::undelete(wid id)
{
    auto& data = find_data(id);
    data.deleted = false;
    g::pDbManager->wf_update_meta(data);
    Log("Wf id=" << id << " restored");
}

std::string WfsManager::add_label(const char* plabel)
{
    auto plabel_low = tools::str::to_lower_ascii(plabel);
    if (plabel_low == "all")
        return "All";

    for (auto& label : m_labels)
    {
        if (tools::str::to_lower_ascii(label) == plabel_low)
            return label;
    }

    constexpr const std::string_view allowed_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_ !?@$%^&*()<>";
    const char* ptr = plabel;
    std::string new_label;
    while (*ptr)
    {
        if (allowed_chars.find(*ptr) != std::string_view::npos)
            new_label += *ptr;
        ptr++;
    }

    if (new_label.empty())
        return {};

    m_labels.push_back(new_label);
    std::sort(m_labels.begin(), m_labels.end(), [](const std::string& l, const std::string& r) { return tools::str::to_lower_ascii(l) < tools::str::to_lower_ascii(r); });

    Log("New wf label: " << new_label);
    return new_label;
}

void WfsManager::delete_label(const char* plabel)
{
    m_labels.erase(std::remove(m_labels.begin(), m_labels.end(), plabel), m_labels.end());
    Log("Label deleted: " << plabel);
}

void WfsManager::add_to_label(wid id, const char* plabel)
{
    auto label = add_label(plabel);
    auto& data = find_data(id);
    if (data.labels.count(label) > 0)
        return;

    data.labels.insert(label);
    data.labels_raw = tools::str::merge(data.labels, '|');
    g::pDbManager->wf_update_meta(data);
    Log("Wf id=" << id << " add to label='" << plabel << "'");
}

void WfsManager::remove_from_label(wid id, const char* plabel)
{
    auto& data = find_data(id);
    if (data.labels.count(plabel) == 0)
        return;

    data.labels.erase(plabel);
    data.labels_raw = tools::str::merge(data.labels, '|');
    g::pDbManager->wf_update_meta(data);
    Log("Wf id=" << id << " removed from label='" << plabel << "'");
}

bool WfsManager::is_in_label(wid id, const char* label) const
{
    auto& data = find_data(id);
    return data.labels.count(label) > 0;
}

data::Workflow* WfsManager::access(wid id) const
{
    return find(id);
}

} // namespace mngr
