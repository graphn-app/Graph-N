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

    void run_host(const blob::HostData& host) override;
    void kill_host(uint64_t id) override;
    void rename_host(uint64_t id, const std::string& new_wf_name, const std::string& new_node_name) override;
    void raise_host(uint64_t id) override;

private:
    using Handle = std::unique_ptr<void, decltype(&::CloseHandle)>;
    struct Host final
    {
        Host()
            : hjob(nullptr, ::CloseHandle)
            , hproc(nullptr, ::CloseHandle)
            , hpipe(nullptr, ::CloseHandle)
        {}
        ~Host() noexcept = default;

        std::atomic_bool has_new_command = false;
        blob::HostData host_data;

        Handle hjob;
        Handle hproc;
        DWORD proc_id = 0;
        Handle hpipe;
    };

private:
    void host_thread(Host& host);
    void terminate_and_cleanup(const Host& host);
    bool is_host_alive(const Host& host) const;

private:
    std::map<uint64_t, Host> m_hosts;
    std::mutex m_mutex;
};

} // namespace mngr

#endif // #ifdef _WIN32
