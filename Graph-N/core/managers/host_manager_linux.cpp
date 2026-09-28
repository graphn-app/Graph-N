#ifdef __linux__
#include "core/managers/host_manager_linux.h"

namespace mngr
{

HostManager_Linux::HostManager_Linux() noexcept = default;
HostManager_Linux::~HostManager_Linux() noexcept = default;

void HostManager_Linux::stop_and_cleanup()
{
#error Not implemented
}

bool HostManager_Linux::init()
{
    if (!HostManager::init())
        return false;

    return true;
}

void HostManager_Linux::run_host(const blob::HostData& /*host*/)
{
#error Not implemented
}

void HostManager_Linux::kill_host(uint64_t /*id*/)
{
#error Not implemented
}

void HostManager_Linux::rename_host(uint64_t id, const std::string& /*new_wf_name*/, const std::string& /*new_node_name*/)
{
#error Not implemented
}

void HostManager_Linux::raise_host(uint64_t /*id*/)
{
#error Not implemented
}

} // namespace mngr

#endif // #ifdef __linux__
