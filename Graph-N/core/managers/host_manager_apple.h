#pragma once
#ifdef __APPLE__
#include "core/managers/host_manager.h"

namespace mngr
{

class HostManager_Apple final : public HostManager
{
public:
    HostManager_Apple() noexcept;
    ~HostManager_Apple() noexcept override;

    bool init() override;
    void stop_and_cleanup() override;

    void run_host(const blob::HostData& host) override;
    void kill_host(uint64_t id) override;
    void rename_host(uint64_t id, const std::string& new_wf_name, const std::string& new_node_name) override;
    void raise_host(uint64_t id) override;
};

} // namespace mngr

#endif // #ifdef __APPLE__
