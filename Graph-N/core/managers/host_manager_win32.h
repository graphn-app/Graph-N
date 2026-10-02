#pragma once
#ifdef _WIN32
#include "core/managers/host_manager.h"

namespace mngr
{

class HostManager_Win32 final : public HostManager
{
public:
    HostManager_Win32() noexcept;
    ~HostManager_Win32() noexcept override;

private:
    std::thread run_host(const blob::HostData& host) override;
    void kill_host(uint64_t id) override;
    void shutdown_host(uint64_t id) override;
    void rename_host(uint64_t id, const std::string& new_wf_name, const std::string& new_node_name) override;
    void raise_host(uint64_t id) override;

    using Handle = std::unique_ptr<void, decltype(&::CloseHandle)>;
    struct HostProcess final
    {
        HostProcess()
            : hjob(nullptr, ::CloseHandle)
            , hproc(nullptr, ::CloseHandle)
            , hpipe(nullptr, ::CloseHandle)
        {}
        ~HostProcess() noexcept = default;

        std::vector<blob::HostData> new_commands;

        Handle hjob;
        Handle hproc;
        DWORD proc_id = 0;
        Handle hpipe;
    };

    void host_thread(blob::HostData host_data);
    uint32_t host_thread_worker(std::shared_ptr<HostProcess> host_proc, const blob::HostData& host_data);
    bool is_host_alive(const HostProcess& host_proc) const;

private:
    std::map<uint64_t, std::shared_ptr<HostProcess>> m_processes;
    std::mutex m_mutex;
};

} // namespace mngr

#endif // #ifdef _WIN32
