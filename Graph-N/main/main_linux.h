/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#ifdef __linux__
#include "main.h"
#include <GLFW/glfw3.h>

namespace platform
{

class MainRoutine_Linux final : public global::MainRoutine
{
    friend class Textures_Linux;

public:
    MainRoutine_Linux() noexcept = default;
    ~MainRoutine_Linux() noexcept override = default;

private:
    /*
        MainRoutine overrides
    */

    bool init() override;
    int32_t run(const flow_callback& run_cb, const cleanup_callback& cleanup_cb, std::atomic_bool& request_focus) override;
    void stop_and_cleanup() override;

    double get_frame_render_time() const noexcept override;
    int64_t get_seconds_elapsed() const noexcept override;
    bool get_ini_config_exists() const override;

    void wnd_restore() override;
    void wnd_minimize() override;
    void wnd_maximize() override;
    void wnd_show() override;
    void wnd_hide() override;
    void wnd_toggle_fullscreen() override;

    GLFWwindow* m_window = nullptr;
    bool m_is_rendering = false;
    double m_frame_render_time = 0.0;
    int64_t m_seconds_elapsed = 0;
    std::chrono::steady_clock::time_point m_second_tick;
};

} // namespace platform

#endif // #ifdef __linux__
