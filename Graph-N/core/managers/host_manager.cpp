#include "core/managers/host_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/base/id.h"
#include "core/workflow/node_instance.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

HostManager::HostManager() noexcept
{
    m_ui_thread_id = std::this_thread::get_id();
}

HostManager::~HostManager() noexcept = default;

void HostManager::stop_all_threads()
{
    m_replies_exists.store(false, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hosts.clear();

    decltype(m_hosts_finished) empty;
    empty.swap(m_hosts_finished);
}

void HostManager::add_host(wid wf_id, iid node_id, HostType type, const std::filesystem::path& image_path)
{
    blob::HostData* existing = find_host(wf_id, node_id);
    if (existing != nullptr)
    {
        if (existing->alive)
            raise_host(existing->host_id);
        return;
    }

    blob::HostData host;
    host.host_id = base::GlobalID::random();
    host.alive = false;

    host.settings = g::pOptsManager->ui_host_settings_load();
    if (HostType::MaskEditor == type)
    {
        host.cmd = blob::HostData::Cmd_MaskEditor;
        host.sam_enabled = g::pFlow->get_online();
    }
    else
    {
        host.cmd = blob::HostData::Cmd_PosEditor;
    }

    host.comfyui_host = g::pOptsManager->comfyui_host_load();
    host.comfyui_port = cast::to_i32(g::pOptsManager->comfyui_port_load());

    LogRAssert(tools::path::exists(image_path), "Source image path not exists: " << image_path, );
    host.source_path = image_path;

    host.dest_path = fs::get_temp_filepath(".png");
    LogRAssert(!host.dest_path.empty(), "Can't generate dest image path", );
    LogRAssert(tools::path::make_file(host.dest_path, false), "Can't create dest image path", );

    auto wf = g::pWfsManager->access(wf_id);
    LogRAssert(wf, "Wf #" << wf_id << " not found", );
    host.workflow_id = wf_id;
    host.wf_name = wf->get_data().name;

    auto node = wf->get_node_by_instance(node_id);
    LogRAssert(node, "Node #" << node_id << " in wf " << wf_id << " not found", );
    host.node_id = node_id;
    host.node_name = node->get_name();

    Log("Starting host. id:" << host.host_id << ", t:" << host.cmd << ", w:" << host.workflow_id << ", n:" << host.node_id << ", src:" << host.source_path << ", dst:" << host.dest_path);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    run_host(host);
    m_hosts.push_back(std::move(host));
}

void HostManager::kill_hosts(wid wf_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& host : m_hosts)
    {
        if (host.workflow_id == wf_id)
        {
            host.alive = false;
            kill_host(host.host_id);
        }
    }
}

void HostManager::kill_host(wid wf_id, iid node_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    blob::HostData* host = find_host(wf_id, node_id);
    if (nullptr == host)
        return;
    kill_host(host->host_id);
    host->alive = false;
}

void HostManager::rename_host(wid wf_id, iid node_id, const std::string& new_wf_name, const std::string& new_node_name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    blob::HostData* host = find_host(wf_id, node_id);
    if (nullptr == host || !host->alive)
        return;

    host->wf_name = new_wf_name;
    host->node_name = new_node_name;
    rename_host(host->host_id, new_wf_name, new_node_name);
}

blob::HostData* HostManager::find_host(wid wf_id, iid node_id)
{
    auto it = std::ranges::find_if(m_hosts, [wf_id, node_id](const blob::HostData& host)
        { return host.workflow_id == wf_id && host.node_id == node_id; });
    if (it == m_hosts.end())
        return nullptr;
    return &*it;
}

blob::HostData* HostManager::find_host(uint64_t id)
{
    auto it = std::ranges::find_if(m_hosts, [id](const blob::HostData& host) { return host.host_id == id; });
    if (it == m_hosts.end())
        return nullptr;
    return &*it;
}

void HostManager::on_host_started(uint64_t id, bool success)
{
    LogRAssert(m_ui_thread_id != std::this_thread::get_id(), "Function must not be called from UI thread", );
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!success) [[unlikely]]
    {
        std::erase_if(m_hosts, [id](const blob::HostData& host) { return id == host.host_id; });
        Log("Host id:" << id << " failed to start");
        return;
    }

    auto host = find_host(id);
    if (nullptr == host)
        return;
    host->alive = true;
    Log("Host id:" << host->host_id << ", w:" << host->workflow_id << ", n:" << host->node_id << " started");
}

void HostManager::on_host_finished_job(uint64_t id, bool success)
{
    LogRAssert(m_ui_thread_id != std::this_thread::get_id(), "Function must not be called from UI thread", );
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto it = m_hosts.begin(); it != m_hosts.end(); ++it)
    {
        auto& host = *it;
        if (host.host_id == id)
        {
            host.alive = false;
            Log("Host id:" << host.host_id << ", w:" << host.workflow_id << ", n:" << host.node_id << " finished: " << success);
            m_hosts_finished.push(std::make_pair(std::move(host), success));
            m_hosts.erase(it);
            m_replies_exists.store(true, std::memory_order_relaxed);
            return;
        }
    }
}

bool HostManager::pickup_finished_host(wid& wf_id, iid& node_id, bool& result, std::filesystem::path& dest_image)
{
    if (!m_replies_exists.load(std::memory_order_relaxed)) [[likely]]
        return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_hosts_finished.empty()) [[unlikely]]
        return false;

    auto& host = m_hosts_finished.front();
    wf_id = host.first.workflow_id;
    node_id = host.first.node_id;
    result = host.second;
    dest_image = std::move(host.first.dest_path);
    m_hosts_finished.pop();
    m_replies_exists.store(!m_hosts_finished.empty(), std::memory_order_relaxed);
    return true;
}

} // namespace mngr
