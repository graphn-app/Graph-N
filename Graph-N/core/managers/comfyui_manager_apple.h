#pragma once
#ifdef __APPLE__
#include "core/managers/comfyui_manager.h"

namespace mngr
{

class ComfyUIManager_Apple final : public ComfyUIManager
{
public:
    ComfyUIManager_Apple() noexcept;
    ~ComfyUIManager_Apple() noexcept override;

    bool init() override;
    void stop_and_cleanup() override;

protected:
    void run_server() override;
};

} // namespace mngr

#endif // #ifdef __APPLE__
