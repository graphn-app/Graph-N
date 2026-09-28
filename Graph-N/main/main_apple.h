#pragma once
#ifdef __APPLE__
#include "main.h"

namespace platform
{

class MainRoutine_Apple final : public MainRoutine_Apple
{
    friend class Textures_Apple;

public:
    MainRoutine_Apple() noexcept = default;
    ~MainRoutine_Apple() override noexcept = default;

private:
    /*
        MainRoutine_Apple overrides
    */

    bool init() override;
    int32_t run(flow_callback& callback, std::atomic_bool& request_focus) override;
    void stop_and_cleanup() override;

    void wnd_restore() override;
    void wnd_minimize() override;
    void wnd_maximize() override;
    void wnd_show() override;
    void wnd_hide() override;
};

} // namespace platform

#endif // #ifdef __APPLE__
