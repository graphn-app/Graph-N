#pragma once
#include "core/managers/manager.h"
#include "tools/host_data_serializer.h"

namespace mngr
{

class HostManager : public Manager
{
public:
    enum class HostType { MaskEditor, PosEditor };

    HostManager() noexcept;
    ~HostManager() noexcept override;

    void stop_all_threads() override;

    void add_host(wid wf_id, iid node_id, HostType type, const std::filesystem::path& image_path);
    void kill_hosts(wid wf_id);
    void kill_host(wid wf_id, iid node_id);
    void rename_host(wid wf_id, iid node_id, const std::string& new_wf_name, const std::string& new_node_name);

    bool pickup_finished_host(wid& wf_id, iid& node_id, bool& result, std::filesystem::path& dest_image);

protected:
    virtual void run_host(const blob::HostData& host) = 0;
    virtual void kill_host(uint64_t id) = 0;
    virtual void rename_host(uint64_t id, const std::string& new_wf_name, const std::string& new_node_name) = 0;
    virtual void raise_host(uint64_t id) = 0;

    void cleanup();
    void on_host_started(uint64_t id, bool success);
    void on_host_finished_job(uint64_t id, bool success);

private:
    blob::HostData* find_host(wid wf_id, iid node_id);
    blob::HostData* find_host(uint64_t id);

private:
    std::mutex m_mutex;
    std::vector<blob::HostData> m_hosts;
    std::queue<std::pair<blob::HostData, bool>> m_hosts_finished;
    std::atomic_bool m_replies_exists;
    std::thread::id m_ui_thread_id;
};

} // namespace mngr
