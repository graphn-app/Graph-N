#pragma once
#include "core/managers/manager.h"
#include "tools/host_data_serializer.h"

namespace mngr
{

class HostManager : public Manager
{
public:
    enum class HostType : uint8_t { MaskEditor, PosEditor };

    HostManager() noexcept;
    ~HostManager() noexcept override;

    void stop_and_cleanup() override;

    void add_host(wid wf_id, iid node_id, HostType type, std::filesystem::path&& source_path, std::filesystem::path&& dest_path, bool can_upload_to_node);
    void kill_hosts(wid wf_id);
    void kill_host(wid wf_id, iid node_id);
    void kill_all();
    void rename_host(wid wf_id, iid node_id, const std::string& new_wf_name, const std::string& new_node_name);

    bool pickup_finished_host(wid& wf_id, iid& node_id, bool& result, std::filesystem::path& dest_image, HostType& type);

protected:
    virtual void run_host(const blob::HostData& host) = 0;
    virtual void kill_host(uint64_t id) = 0;
    virtual void rename_host(uint64_t id, const std::string& new_wf_name, const std::string& new_node_name) = 0;
    virtual void raise_host(uint64_t id) = 0;

    std::tuple<bool, bool, uint64_t> check_opened(wid wf_id, iid node_id);
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
    std::atomic_bool m_replies_exists = false;
    std::atomic_bool m_do_cleanups = false;
    std::thread::id m_ui_thread_id;
};

} // namespace mngr
