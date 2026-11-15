/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#ifdef __linux__
#include "core/managers/host_manager_linux.h"

namespace mngr
{

HostManager_Linux::HostManager_Linux() noexcept = default;
HostManager_Linux::~HostManager_Linux() noexcept = default;

void HostManager_Linux::stop_and_cleanup()
{
    // Host processes are a Windows-only feature (WebView2 host).
    // On Linux hosts are not available: nothing to clean up.
}

bool HostManager_Linux::init()
{
    if (!HostManager::init())
        return false;

    return true;
}

std::thread HostManager_Linux::run_host(const blob::HostData& /*host*/)
{
    // Host processes are a Windows-only feature (WebView2 host).
    return std::thread();
}

void HostManager_Linux::kill_host(uint64_t /*id*/)
{
}

void HostManager_Linux::shutdown_host(uint64_t /*id*/)
{
}

void HostManager_Linux::rename_host(uint64_t /*id*/, const std::string& /*new_wf_name*/, const std::string& /*new_node_name*/)
{
}

void HostManager_Linux::raise_host(uint64_t /*id*/)
{
}

} // namespace mngr

#endif // #ifdef __linux__
