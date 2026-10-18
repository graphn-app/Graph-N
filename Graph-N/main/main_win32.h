#pragma once
#ifdef _WIN32
#include "main.h"
#include <d3d11.h>

namespace platform
{

#define USER_MESSAGE        (WM_APP + 1)
using UserMsgCallback = std::function<void(WPARAM, LPARAM)>;

class MainRoutine_Win32 final : public global::MainRoutine
{
    friend class Textures_Win32;
    friend class VideoControl_Win32;

public:
    MainRoutine_Win32() noexcept;
    ~MainRoutine_Win32() noexcept override;

    double get_frame_render_time() const noexcept override { return m_frame_render_time; }
    HWND get_hwnd() const noexcept { return m_hwnd; }
    void set_user_msg_callback(UserMsgCallback callback) { m_user_msg_callback = callback; }
    bool get_ini_config_exists() const noexcept override { return m_ini_config_exists; }

private:
    /*
        MainRoutine overrides
    */
    bool init() override;
    int32_t run(const flow_callback& run_cb, const cleanup_callback& cleanup_cb, std::atomic_bool& request_focus) override;
    void stop_and_cleanup() override;

    void wnd_restore() noexcept override;
    void wnd_minimize() noexcept override;
    void wnd_maximize() noexcept override;
    void wnd_show() noexcept override;
    void wnd_hide() noexcept override;
    void wnd_toggle_fullscreen() noexcept override;

private:
    ID3D11Device* get_device() const noexcept { return m_d3d_device.get(); }

    bool render_frame(bool done);
    bool create_device_d3d(HWND hWnd);
    void cleanup_device_d3d() noexcept;
    bool create_render_target();
    void cleanup_render_target() noexcept;
    void calc_current_monitor_refresh_rate() noexcept;

    static LRESULT WINAPI wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void wake_up() noexcept;

private:
    UserMsgCallback m_user_msg_callback = nullptr;

    // DX11
    std::unique_ptr<ID3D11Device, std::function <void(ID3D11Device*)>> m_d3d_device;
    std::unique_ptr<ID3D11DeviceContext, std::function <void(ID3D11DeviceContext*)>> m_d3d_device_context;
    std::unique_ptr<IDXGISwapChain, std::function <void(IDXGISwapChain*)>> m_swap_chain;
    std::unique_ptr<ID3D11RenderTargetView, std::function <void(ID3D11RenderTargetView*)>> m_main_render_target_view;
    
    // win32
    WNDCLASSEXW m_wc = {};
    HWND m_hwnd = nullptr;
    std::unique_ptr<HICON__, decltype(&DestroyIcon)> m_hicon;
    bool m_moving = false;

    // ImGui
    const ImVec4 m_clear_color;
    bool m_ini_config_exists = false;

    // Graph-N
    const cleanup_callback* m_cleanup_callback = nullptr;
    const flow_callback* m_run_callback = nullptr;

    // fps
    double m_frame_render_time = 0;
    HMONITOR m_last_monitor = nullptr;

    static int64_t m_seconds_elapsed;

    WINDOWPLACEMENT m_fullscreen_pl = {};
    bool m_is_rendering = false;
    UINT m_pending_resize_w = 0;
    UINT m_pending_resize_h = 0;
};

} // namespace platform

#endif // #ifdef _WIN32
