/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#ifdef __APPLE__
#include "core/managers/host_manager_apple.h"

namespace mngr
{

HostManager_Apple::HostManager_Apple() noexcept = default;
HostManager_Apple::~HostManager_Apple() noexcept = default;

void HostManager_Apple::stop_and_cleanup()
{
#error Not implemented
}

bool HostManager_Apple::init()
{
    if (!ComfyUIManager::init())
        return false;

    return true;
}

void HostManager_Apple::run_host(const blob::HostData& /*host*/)
{
#error Not implemented
}

void HostManager_Apple::kill_host(uint64_t /*id*/)
{
#error Not implemented
}

void HostManager_Apple::rename_host(uint64_t id, const std::string& /*new_wf_name*/, const std::string& /*new_node_name*/)
{
#error Not implemented
}

void HostManager_Apple::raise_host(uint64_t /*id*/)
{
#error Not implemented
}

} // namespace mngr

#endif // #ifdef __APPLE__
