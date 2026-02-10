
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class ComfyUIManager : public Manager
{
public:
    ComfyUIManager();
    ~ComfyUIManager() override;

    void run();

protected:
    virtual void run_server() = 0;
};

} // namespace mngr
