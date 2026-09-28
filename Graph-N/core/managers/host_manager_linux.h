#pragma once
#ifdef __linux__
#include "core/managers/host_manager.h"

namespace mngr
{

class HostManager_Linux final : public HostManager
{
public:
    HostManager_Linux() noexcept;
    ~HostManager_Linux() noexcept override;

    bool init() override;
    void stop_and_cleanup() override;

    void run_host(const blob::HostData& host) override;
    void kill_host(uint64_t id) override;
    void rename_host(uint64_t id, const std::string& new_wf_name, const std::string& new_node_name) override;
    void raise_host(uint64_t id) override;
};

} // namespace mngr

#endif // #ifdef __linux__
