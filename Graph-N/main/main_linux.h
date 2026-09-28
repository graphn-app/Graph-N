#pragma once
#ifdef __linux__
#include "main.h"

namespace platform
{

class MainRoutine_Linux final : public global::MainRoutine
{
    friend class Textures_Linux;

public:
    MainRoutine_Linux() noexcept = default;
    ~MainRoutine_Linux() override noexcept = default;

private:
    /*
        MainRoutine overrides
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

#endif // #ifdef __linux__
