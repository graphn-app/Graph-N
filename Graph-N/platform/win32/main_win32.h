
#pragma once
#ifdef _WIN32

#include "platform/main.h"
#include <d3d11.h>

namespace platform
{

class MainRoutine_Win32 final : public MainRoutine
{
    friend class Textures_Win32;

public:
    MainRoutine_Win32();
    ~MainRoutine_Win32() override;

    inline double get_frame_render_time() const override { return m_frame_render_time; }

private:
    /*
        MainRoutine overrides
    */
    bool init() override;
    int run(flow_callback& callback, std::atomic_bool& request_focus) override;
    void free() override;

    void wnd_restore() override;
    void wnd_minimize() override;
    void wnd_maximize() override;
    void wnd_show() override;
    void wnd_hide() override;

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

    // Graph-N
    flow_callback* m_callback = nullptr;

    // fps
    double m_frame_render_time = 0;
};

} // namespace platform

#endif // #ifdef _WIN32
