
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class ComfyUIManager : public Manager
{
public:
    ComfyUIManager();
    ~ComfyUIManager() override;

    void run(bool force_start);

    virtual bool check_existing_process() const = 0;
    virtual bool get_process_started() const = 0;
    virtual uint32_t get_started_proc_id() const = 0;
    virtual void kill() = 0;

protected:
    virtual void run_server(bool force_start) = 0;
};

} // namespace mngr
