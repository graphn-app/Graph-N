
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class ComfyUIManager final : public Manager
{
public:
    ComfyUIManager();
    ~ComfyUIManager() override;

    bool init() override;
    void free() override;
    void stop_all_threads() override;

    void run();

};

} // namespace mngr
