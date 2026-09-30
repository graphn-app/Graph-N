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

void HostManager::stop_and_cleanup()
{
    kill_all();
    for (auto& [id, th] : m_threads)
    {
        if (th.joinable())
            th.join();
    }
    m_threads.clear();

    m_replies_exists.store(false, std::memory_order_relaxed);
    m_do_cleanups.store(false, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_hosts.clear();
    decltype(m_hosts_finished) empty;
    empty.swap(m_hosts_finished);
}

namespace
{
void append_origin(std::string& settings, bool can_upload_to_node)
{
    try
    {
        auto jsettings = u_json::parse(settings);
        if (can_upload_to_node)     jsettings["origin"] = "input";
        else                        jsettings["origin"] = "output";
        settings = jsettings.dump();
        return;
    }
    catch (const std::exception& ex)
    {
        LogExcepS("Host settings parse err:" << ex.what());
    }

    if (can_upload_to_node)     settings = R"({"origin": "input"})";
    else                        settings = R"({"origin": "output"})";
}
} // namespace

void HostManager::add_host(wid wf_id, iid node_id, HostType type, std::filesystem::path&& source_path, std::filesystem::path&& dest_path, bool can_upload_to_node)
{
    const auto existing = check_opened(wf_id, node_id);
    if (std::get<0>(existing)) // already exists
    {
        if (std::get<1>(existing)) // host alive and can be raised
        {
            Log("Host #" << std::get<2>(existing) << ": raise");
            raise_host(std::get<2>(existing)); // host id
        }
        else
        {
            Log("Host #" << std::get<2>(existing) << ": raise blocked - not alive");
        }
        return;
    }

    blob::HostData host;
    host.host_id = base::GlobalID::random();
    host.alive = false;
    host.can_upload_to_node = can_upload_to_node;

    host.settings = g::pOptsManager->ui_host_settings_load();
    append_origin(host.settings, can_upload_to_node);

    if (HostType::MaskEditor == type)
    {
        host.cmd = blob::HostData::Cmd_MaskEditor;
        host.sam_enabled = g::pFlow->get_online();
    }
    else if (HostType::PosEditor == type)
    {
        host.cmd = blob::HostData::Cmd_PosEditor;
    }
    else
    {
        LogRError("Host #00: Unhandled editor type", );
    }

    host.comfyui_host = g::pOptsManager->comfyui_host_load();
    host.comfyui_port = cast::to_i32(g::pOptsManager->comfyui_port_load());

    LogRAssert(tools::path::exists(source_path), "Host #00: Source image path not exists: " << source_path, );
    LogRAssert(tools::path::exists(dest_path), "Host #00: Dest image path not exists: " << dest_path, );
    host.source_path = std::move(source_path);
    host.dest_path = std::move(dest_path);

    auto wf = g::pWfsManager->access(wf_id);
    LogRAssert(wf, "Host #00: Wf:" << wf_id << " not found", );
    host.workflow_id = wf_id;
    host.wf_name = wf->get_data().name;

    auto node = wf->get_node_by_instance(node_id);
    LogRAssert(node, "Host #00: Node:" << node_id << ", wf:" << wf_id << ". Node not found", );
    host.node_id = node_id;
    host.node_name = node->get_name();

    Log("Host #" << host.host_id << ": Start. t:" << host.cmd << ", w:" << host.workflow_id << ", n:" << host.node_id << ", src:" << host.source_path << ", dst:" << host.dest_path);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hosts.push_back(std::move(host));

    auto& ref = m_hosts.back();
    m_threads[ref.host_id] = run_host(ref);
}

std::tuple<bool, bool, uint64_t> HostManager::check_opened(wid wf_id, iid node_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    blob::HostData* existing = find_host(wf_id, node_id);
    if (existing != nullptr)
        return std::make_tuple(true, existing->alive, existing->host_id);
    return std::make_tuple(false, false, 0);
}

void HostManager::kill_all()
{
    bool has_data = false;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& host : m_hosts)
    {
        if (!host.cleanup)
        {
            kill_host(host.host_id);
            host.alive = false;
            host.cleanup = true;
            m_hosts_finished.push(std::make_pair(host, false));

            has_data = true;
        }
    }
    if (has_data)
    {
        m_replies_exists.store(true, std::memory_order_relaxed);
        m_do_cleanups.store(true, std::memory_order_relaxed);
    }
}

void HostManager::kill_hosts(wid wf_id)
{
    bool has_data = false;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& host : m_hosts)
    {
        if (host.workflow_id == wf_id && !host.cleanup)
        {
            kill_host(host.host_id);
            host.alive = false;
            host.cleanup = true;
            m_hosts_finished.push(std::make_pair(host, false));

            has_data = true;
        }
    }

    if (has_data)
    {
        m_do_cleanups.store(true, std::memory_order_relaxed);
        m_replies_exists.store(true, std::memory_order_relaxed);
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
    host->cleanup = true;
    m_do_cleanups.store(true, std::memory_order_relaxed);

    m_hosts_finished.push(std::make_pair(*host, false));
    m_replies_exists.store(true, std::memory_order_relaxed);
}

void HostManager::rename_host(wid wf_id, iid node_id, const std::string& new_wf_name, const std::string& new_node_name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& host : m_hosts)
    {
        if (host.alive && host.workflow_id == wf_id)
        {
            if (host.node_id == node_id || 0 == node_id)
            {
                if (!new_wf_name.empty())       host.wf_name = new_wf_name;
                if (!new_node_name.empty())     host.node_name = new_node_name;
                rename_host(host.host_id, host.wf_name, host.node_name);
            }
        }
    }
}

blob::HostData* HostManager::find_host(wid wf_id, iid node_id)
{
    auto it = std::ranges::find_if(m_hosts, [wf_id, node_id](const blob::HostData& host)
        { return host.workflow_id == wf_id && host.node_id == node_id && !host.cleanup; });
    if (it == m_hosts.end())
        return nullptr;
    return &*it;
}

blob::HostData* HostManager::find_host(uint64_t id)
{
    auto it = std::ranges::find_if(m_hosts, [id](const blob::HostData& host) { return host.host_id == id && !host.cleanup; });
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
        for (auto& host : m_hosts)
        {
            if (host.host_id == id)
            {
                host.alive = false;
                host.cleanup = true;
                m_do_cleanups.store(true, std::memory_order_relaxed);

                m_hosts_finished.push(std::make_pair(host, false));
                m_replies_exists.store(true, std::memory_order_relaxed);
                break;
            }
        }
        Log("Host #" << id << ": failed to start");
        return;
    }

    auto host = find_host(id);
    LogRAssert(host, "Host #" << id << ": started, but wasn't found in map", );
    host->alive = true;
    Log("Host #" << host->host_id << ": started. w:" << host->workflow_id << ", n:" << host->node_id);
}

void HostManager::on_host_finished_job(uint64_t id, bool success)
{
    LogRAssert(m_ui_thread_id != std::this_thread::get_id(), "Host #" << id << ": function must not be called from UI thread", );
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto it = m_hosts.begin(); it != m_hosts.end(); ++it)
    {
        auto& host = *it;
        if (host.host_id == id)
        {
            Log("Host #" << host.host_id << ": finished. w:" << host.workflow_id << ", n:" << host.node_id << ", success: " << success);
            m_hosts_finished.push(std::make_pair(host, success));
            host.alive = false;
            host.cleanup = true;
            m_replies_exists.store(true, std::memory_order_relaxed);
            m_do_cleanups.store(true, std::memory_order_relaxed);
            break;
        }
    }
}

bool HostManager::pickup_finished_host(wid& wf_id, iid& node_id, bool& result, std::filesystem::path& dest_image, HostType& type)
{
    std::list<std::pair<wid, iid>> failed;
    if (m_do_cleanups.load(std::memory_order_relaxed)) [[unlikely]]
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::erase_if(m_hosts, [this](const blob::HostData& host)
        {
            if (host.cleanup)
            {
                auto it = m_threads.find(host.host_id);
                if (it != m_threads.end())
                {
                    auto& th = it->second;
                    if (th.joinable())
                        th.detach();
                    m_threads.erase(it);
                }
                return true;
            }
            return false;
        });
        m_do_cleanups.store(false, std::memory_order_relaxed);
    }

    if (!m_replies_exists.load(std::memory_order_relaxed)) [[likely]]
        return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_hosts_finished.empty()) [[unlikely]]
        return false;

    auto& host = m_hosts_finished.front();
    wf_id = host.first.workflow_id;
    node_id = host.first.node_id;
    result = host.second;
    switch (host.first.cmd)
    {
    case blob::HostData::Cmd_MaskEditor: type = HostType::MaskEditor; break;
    case blob::HostData::Cmd_PosEditor: type = HostType::PosEditor; break;
    default: LogRError("Host #" << host.first.host_id << ": Unhandled editor type", false);
    }
    dest_image = std::move(host.first.dest_path);
    m_hosts_finished.pop();
    m_replies_exists.store(!m_hosts_finished.empty(), std::memory_order_relaxed);
    return true;
}

} // namespace mngr
