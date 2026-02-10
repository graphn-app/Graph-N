
#pragma once
#ifdef __APPLE__

#include "main.h"

namespace platform
{

class MainRoutine_Apple final : public MainRoutine_Apple
{
    friend class Textures_Apple;

public:
    MainRoutine_Apple() = default;
    ~MainRoutine_Apple() override = default;

private:
    /*
        MainRoutine_Apple overrides
    */

    bool init() override;
    int32_t run(flow_callback& callback, std::atomic_bool& request_focus) override;
    void free() override;

    void wnd_restore() override;
    void wnd_minimize() override;
    void wnd_maximize() override;
    void wnd_show() override;
    void wnd_hide() override;
};

} // namespace platform

#endif // #ifdef __APPLE__
