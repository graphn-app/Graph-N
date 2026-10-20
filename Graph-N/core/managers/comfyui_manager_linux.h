/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#ifdef __linux__
#include "core/managers/comfyui_manager.h"

namespace mngr
{

class ComfyUIManager_Linux final : public ComfyUIManager
{
public:
    ComfyUIManager_Linux() noexcept;
    ~ComfyUIManager_Linux() noexcept override;

    bool init() override;
    void stop_and_cleanup() override;

protected:
    void run_server() override;
};

} // namespace mngr

#endif // #ifdef __linux__
