
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
    MainRoutine_Win32();
    ~MainRoutine_Win32() override;

    inline double get_frame_render_time() const override { return m_frame_render_time; }
    inline HWND get_hwnd() const { return m_hwnd; }
    inline void set_user_msg_callback(UserMsgCallback callback) { m_user_msg_callback = callback; }
    bool get_ini_config_exists() const override { return m_ini_config_exists; }

private:
    /*
        MainRoutine overrides
    */
    bool init() override;
    int32_t run(flow_callback& callback, std::atomic_bool& request_focus) override;
    void free() override;

    void wnd_restore() override;
    void wnd_minimize() override;
    void wnd_maximize() override;
    void wnd_show() override;
    void wnd_hide() override;
    void wnd_toggle_fullscreen() override;

private:
    ID3D11Device* get_device() const { return m_d3d_device.get(); }

    bool render_frame(bool done);
    bool create_device_d3d(HWND hWnd);
    void cleanup_device_d3d();
    bool create_render_target();
    void cleanup_render_target();
    void init_imgui_diloag_texture();

    static LRESULT WINAPI wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void wake_up();

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
    flow_callback* m_callback = nullptr;

    // fps
    double m_frame_render_time = 0;

    static int64_t m_seconds_elapsed;

    WINDOWPLACEMENT m_fullscreen_pl;
    bool m_is_rendering = false;
    UINT m_pending_resize_w = 0;
    UINT m_pending_resize_h = 0;
};

} // namespace platform

#endif // #ifdef _WIN32
