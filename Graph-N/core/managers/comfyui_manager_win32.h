
#pragma once
#ifdef _WIN32
#include "core/managers/comfyui_manager.h"

namespace mngr
{

class ComfyUIManager_Win32 final : public ComfyUIManager
{
public:
    ComfyUIManager_Win32();
    ~ComfyUIManager_Win32() override;

    bool init() override;
    void free() override;
    void stop_all_threads() override;

protected:
    bool run_server() override;
};

} // namespace mngr

#endif // #ifdef _WIN32
