/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#ifdef _WIN32
#include "main/main_win32.h"
#include "core/base/version.h"
#include "ui/helpers/textures.h"
#include "tools/exception_handler.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <d3d11_4.h>
#include <wrl/client.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace platform
{

using Microsoft::WRL::ComPtr;

int64_t MainRoutine_Win32::m_seconds_elapsed = 0;

MainRoutine_Win32::MainRoutine_Win32() noexcept
    : m_d3d_device(nullptr, [](ID3D11Device* p) { if (p) p->Release(); })
    , m_d3d_device_context(nullptr, [](ID3D11DeviceContext* p) { if (p) p->Release(); })
    , m_swap_chain(nullptr, [](IDXGISwapChain* p) { if (p) p->Release(); })
    , m_main_render_target_view(nullptr, [](ID3D11RenderTargetView* p) { if (p) p->Release(); })
    , m_hicon(nullptr, ::DestroyIcon)
    , m_clear_color(0.45f, 0.55f, 0.60f, 1.00f)
{}

MainRoutine_Win32::~MainRoutine_Win32() noexcept = default;

bool MainRoutine_Win32::init()
{
    tools::set_exception_translator();

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    LogCriticalIf(SUCCEEDED(hr), "Failed to initialize COM, err: " << LogHRES(hr));

    hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
    LogCriticalIf(SUCCEEDED(hr), "Failed to initialize COM security, err: " << LogHRES(hr));

    m_wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, MainRoutine_Win32::wnd_proc, 0L, 0L, ::GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, APP_CAPTION_W, nullptr };
    m_wc.hbrBackground = nullptr;
    ::RegisterClassExW(&m_wc);

    const int32_t screen_w = ::GetSystemMetrics(SM_CXSCREEN);
    const int32_t screen_y = ::GetSystemMetrics(SM_CYSCREEN);

    constexpr auto initial_width = 1024;
    constexpr auto initial_height = 768;
    const auto initial_left = screen_w / 2 - initial_width / 2;
    const auto initial_top = screen_y / 2 - initial_height / 2;

    const std::string caption = fmt::format(FMT_STRING("{} {}"), APP_CAPTION, ver::ver_as_caption());
    const std::wstring caption_w = fmt::format(FMT_STRING(L"{} {}"), APP_CAPTION_W, tools::str::utf8_to_wide(ver::ver_as_caption()));

    m_hwnd = ::CreateWindowW(m_wc.lpszClassName, caption_w.c_str(), WS_OVERLAPPEDWINDOW | WS_MAXIMIZE,
        initial_left, initial_top, initial_width, initial_height, nullptr, nullptr,
        m_wc.hInstance, nullptr);
    LogCriticalIf(m_hwnd, "Failed to create main window, err " << LogApiERR);

    // Initialize Direct3D
    if (!create_device_d3d(m_hwnd))
    {        
        cleanup_device_d3d();
        ::UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
        LogCritical("Create_device_d3d failed with err: " << LogApiERR);
    }

    m_hicon.reset((HICON)::LoadImage(m_wc.hInstance, MAKEINTRESOURCE(101), IMAGE_ICON, 32, 32, LR_LOADTRANSPARENT));
    ::SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hicon.get());
    ::SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hicon.get());

    BOOL dark = TRUE;
    ::DwmSetWindowAttribute(m_hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));

    ::ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
    ::UpdateWindow(m_hwnd);
    calc_current_monitor_refresh_rate();

    ::SetTimer(m_hwnd, 1, 1000, nullptr);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();


    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    io.ConfigViewportsNoDecoration = false;
    io.IniFilename = APP_CAPTION_LOW ".ini";
    io.LogFilename = nullptr;
    strcpy(io.Caption, caption.c_str());
    m_ini_config_exists = tools::path::exists(fs::get_root_directory_path() / io.IniFilename);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX11_Init(m_d3d_device.get(), m_d3d_device_context.get());

    // font and icons
    prepare_fonts();

    // app obj
    ::SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    return true;
}

int32_t MainRoutine_Win32::run(const flow_callback& run_cb, const cleanup_callback& cleanup_cb, std::atomic_bool& request_focus)
{
    LogCriticalIf(run_cb && cleanup_cb, "Empty callbacks");
    m_run_callback = &run_cb;
    m_cleanup_callback = &cleanup_cb;

    // Main loop
    bool done = false;
    while (!done)
    {
        // wait for evert or timer, but not longer than 20ms
        ::MsgWaitForMultipleObjectsEx(0, nullptr, 20, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

        // Poll and handle messages (inputs, window resize, etc.)
        // See the wnd_proc() function below for our to dispatch events to the Win32 backend.
        ::MSG msg;
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (WM_QUIT == msg.message)
                    done = true;
            }
        }

        if (!render_frame(done)) [[unlikely]]
            done = true;

        if (request_focus.load(std::memory_order_relaxed))
        {
            wake_up();
            request_focus.store(false, std::memory_order_relaxed);
        }
    }

    m_run_callback = nullptr;
    m_cleanup_callback = nullptr;
    return 0;
}

bool MainRoutine_Win32::render_frame(bool done)
{
    // apply pending resize buffers before starting a new frame
    if (m_pending_resize_w > 0 && m_pending_resize_h > 0)
    {
        cleanup_render_target();
        m_swap_chain->ResizeBuffers(0, m_pending_resize_w, m_pending_resize_h, DXGI_FORMAT_UNKNOWN, 0);
        create_render_target();
        m_pending_resize_w = 0;
        m_pending_resize_h = 0;
    }

    if (!m_main_render_target_view) [[unlikely]]
    {        
        ::PostQuitMessage(1);
        LogCritical("Render target is null");
    }
    
    (*m_cleanup_callback)(); // cleanup last frame - prepare for the next one

    m_is_rendering = true; // lock re-entrancy
    const auto start_tick = std::chrono::high_resolution_clock::now();

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    const bool ret_value = (*m_run_callback)(done);  // main Graph-N flow

    // rendering
    ImGui::Render();

    const float clear_color_with_alpha[4] = { m_clear_color.x * m_clear_color.w, m_clear_color.y * m_clear_color.w, m_clear_color.z * m_clear_color.w, m_clear_color.w };
    const auto target_view = m_main_render_target_view.get();
    if (target_view)
    {
        m_d3d_device_context->OMSetRenderTargets(1, &target_view, nullptr);
        m_d3d_device_context->ClearRenderTargetView(target_view, clear_color_with_alpha);
    }
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    const auto end_tick = std::chrono::high_resolution_clock::now();
    m_frame_render_time = std::chrono::duration<double, std::milli>(end_tick - start_tick).count();

    const UINT32 vsync = (get_vsync_enabled() && !m_moving ? 1 : 0);    // force vsync off when moving
    const HRESULT hr = m_swap_chain->Present(vsync, 0); 
    if (FAILED(hr))
    {
        ::PostQuitMessage(1);
        LogCritical("Device lost on Present: " << LogHRES(hr));
    }
    if (DXGI_STATUS_OCCLUDED == hr)
    {
        // window is fully covered by other windows.
        // sleep to release CPU and GPU resources.
        ::Sleep(10); 
    }

    m_monitor_dpi = ImGui::GetMainViewport()->DpiScale;
    m_is_rendering = false; // unlock
    return ret_value;
}

void MainRoutine_Win32::stop_and_cleanup()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    cleanup_device_d3d();

    if (m_hwnd != nullptr)
        ::DestroyWindow(m_hwnd);
    if (m_wc.hInstance != nullptr)
        ::UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);

    CoUninitialize();
}

bool MainRoutine_Win32::create_device_d3d(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ::ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr UINT create_device_flags = 0; // D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT; // | D3D11_CREATE_DEVICE_DEBUG
    D3D_FEATURE_LEVEL feature_level;
    constexpr D3D_FEATURE_LEVEL feature_level_arr[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

    ComPtr<ID3D11Device> d3d_device;
    ComPtr<ID3D11DeviceContext> d3d_device_context;
    ComPtr<IDXGISwapChain> swap_chain;

    HRESULT hr = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, feature_level_arr, 2, D3D11_SDK_VERSION, &sd, swap_chain.GetAddressOf(), d3d_device.GetAddressOf(), &feature_level, d3d_device_context.GetAddressOf());
    if (DXGI_ERROR_UNSUPPORTED == hr) // Try high-performance WARP software driver if hardware is not available.
        hr = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, create_device_flags, feature_level_arr, 2, D3D11_SDK_VERSION, &sd, swap_chain.GetAddressOf(), d3d_device.GetAddressOf(), &feature_level, d3d_device_context.GetAddressOf());
    LogCriticalIf(SUCCEEDED(hr), "Failed D3D11CreateDevice with error: " << LogHRES(hr));

    //ComPtr<ID3D11Multithread> multithread;
    //if (SUCCEEDED(d3d_device->QueryInterface(IID_PPV_ARGS(multithread.GetAddressOf()))))
    //{
    //    multithread->SetMultithreadProtected(TRUE);
    //}

    m_d3d_device.reset(d3d_device.Detach());
    m_d3d_device_context.reset(d3d_device_context.Detach());
    m_swap_chain.reset(swap_chain.Detach());

    // only 1 frame in buffer
    ComPtr<IDXGIDevice1> dxgi_device;
    if (SUCCEEDED(m_d3d_device->QueryInterface(__uuidof(IDXGIDevice1), (void**)dxgi_device.GetAddressOf())))
    {
        dxgi_device->SetMaximumFrameLatency(1);
        dxgi_device.Reset();
    }

    return create_render_target();
}

void MainRoutine_Win32::cleanup_device_d3d() noexcept
{
    cleanup_render_target();

    m_swap_chain.reset();
    m_d3d_device_context.reset();
    m_d3d_device.reset();
}

void MainRoutine_Win32::cleanup_render_target() noexcept
{
    m_main_render_target_view.reset();
}

bool MainRoutine_Win32::create_render_target()
{
    ComPtr<ID3D11Texture2D> back_buffer;
    HRESULT hr = m_swap_chain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
    LogCriticalIf(SUCCEEDED(hr) && back_buffer, "IDXGISwapChain::GetBuffer failed: " << LogHRES(hr));

    std::unique_ptr<ID3D11Texture2D, std::function <void(ID3D11Texture2D*)>> back_buffer_ptr(back_buffer.Detach(), [](ID3D11Texture2D* p) { if (p) p->Release(); });

    ComPtr<ID3D11RenderTargetView> target_view;
    hr = m_d3d_device->CreateRenderTargetView(back_buffer_ptr.get(), nullptr, target_view.GetAddressOf());
    LogCriticalIf(SUCCEEDED(hr) && target_view, "ID3D11Device::CreateRenderTargetView failed: " << LogHRES(hr));

    m_main_render_target_view.reset(target_view.Detach());
    return true;
}

namespace
{
MainRoutine_Win32* from_user_data(HWND hWnd)
{
    auto user_data = ::GetWindowLongPtr(hWnd, GWLP_USERDATA);
    return cast::ptrs<MainRoutine_Win32>(user_data);
}
} // namespace

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI MainRoutine_Win32::wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() != nullptr && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_CREATE:
        DragAcceptFiles(hWnd, TRUE);
        return 0;
    case WM_ENTERSIZEMOVE:
        {
            auto routine = from_user_data(hWnd);
            if (routine != nullptr)
                routine->m_moving = true;
        }
        break;
    case WM_EXITSIZEMOVE:
        {
            auto routine = from_user_data(hWnd);
            if (routine != nullptr)
                routine->m_moving = false;
        }
        break;
    case WM_SIZE:
        {
            if (SIZE_MINIMIZED == wParam)
            {
                if (g::pGraphN != nullptr)
                    g::pGraphN->handle_wnd_minimized();
                return 0;
            }

            auto routine = from_user_data(hWnd);
            if (routine != nullptr)
            {
                // defer resize if it was triggered programmatically from within the UI loop
                if (routine->m_is_rendering)
                {
                    routine->m_pending_resize_w = (UINT)LOWORD(lParam);
                    routine->m_pending_resize_h = (UINT)HIWORD(lParam);
                }
                else
                {
                    // Safe to resize immediately during OS drag/resize events
                    routine->cleanup_render_target();
                    routine->m_swap_chain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                    if (!routine->create_render_target())
                        ::PostQuitMessage(1);
                    else
                        routine->render_frame(false /*done*/);
                }
            }

            if (SIZE_RESTORED == wParam && g::pGraphN != nullptr)
            {
                g::pGraphN->handle_wnd_restored();
            }
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        if (SC_CLOSE == wParam && g::pGraphN != nullptr)
        {
            LogS("Exit by pressing X button");
            g::pGraphN->handle_wnd_on_close();
            return 0;
        }
        break;
    case WM_DESTROY:
        DragAcceptFiles(hWnd, FALSE);
        ::PostQuitMessage(0);
        return 0;
    case WM_TIMER:
        if (g::pGraphN != nullptr)
            g::pGraphN->handle_1sec_timer(++m_seconds_elapsed);
        break;
    case USER_MESSAGE:
        {
            auto routine = from_user_data(hWnd);
            if (routine != nullptr && routine->m_user_msg_callback != nullptr)
            {
                routine->m_user_msg_callback(wParam, lParam);
                routine->m_user_msg_callback = nullptr;
            }
        }
        return 0;
    case WM_DROPFILES:
        {
            auto routine = from_user_data(hWnd);
            if (routine != nullptr && routine->m_dnd_callback != nullptr)
            {
                HDROP hdrop = (HDROP)wParam;
                const UINT file_count = DragQueryFileA(hdrop, 0xFFFFFFFF, nullptr, 0);
                std::list<std::filesystem::path> files;

                for (UINT i = 0; i < file_count; i++)
                {
                    char file_path[MAX_PATH];
                    if (DragQueryFileA(hdrop, i, file_path, MAX_PATH))
                    {
                        files.push_back(file_path);
                    }
                }

                DragFinish(hdrop);
                routine->m_dnd_callback(std::move(files));
            }
        }
        return 0;

    case WM_WINDOWPOSCHANGED:
        {
            auto routine = from_user_data(hWnd);
            if (routine != nullptr)
                routine->calc_current_monitor_refresh_rate();
        }
        break;

    default: [[likely]]
        break;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void MainRoutine_Win32::wake_up() noexcept
{
    Log("Wake up command");
    if (::IsIconic(m_hwnd))
    {
        wnd_restore();
    }
    else
    {
        ::BringWindowToTop(m_hwnd);
        ::SetForegroundWindow(m_hwnd);
    }
}

void MainRoutine_Win32::wnd_restore() noexcept
{
    ::ShowWindow(m_hwnd, SW_RESTORE);
    ::SetForegroundWindow(m_hwnd);
    ::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_INTERNALPAINT);
}

void MainRoutine_Win32::wnd_minimize() noexcept
{
    ::ShowWindow(m_hwnd, SW_MINIMIZE);
}

void MainRoutine_Win32::wnd_maximize() noexcept
{
    WINDOWPLACEMENT pl = {};
    ::GetWindowPlacement(m_hwnd, &pl);
    pl.showCmd = SW_SHOWMAXIMIZED;
    ::SetWindowPlacement(m_hwnd, &pl);
}

void MainRoutine_Win32::wnd_show() noexcept
{
    ::ShowWindow(m_hwnd, SW_SHOW);
    ::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_INTERNALPAINT);
}

void MainRoutine_Win32::wnd_hide() noexcept
{
    ::ShowWindow(m_hwnd, SW_HIDE);
}

void MainRoutine_Win32::wnd_toggle_fullscreen() noexcept
{
    DWORD styles = ::GetWindowLong(m_hwnd, GWL_STYLE);

    if (get_is_fullscreen())
    {
        styles |= WS_OVERLAPPEDWINDOW;
        styles &= ~WS_POPUP;
        ::SetWindowLong(m_hwnd, GWL_STYLE, styles);
        ::SetWindowPlacement(m_hwnd, &m_fullscreen_pl);

        // SWP_FRAMECHANGED is critical here to tell Windows to recalculate the non-client area
        ::SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
    else
    {
        ::GetWindowPlacement(m_hwnd, &m_fullscreen_pl);

        // get the specific monitor the window is currently on
        HMONITOR h_monitor = ::MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        if (::GetMonitorInfo(h_monitor, &mi))
        {
            styles &= ~WS_OVERLAPPEDWINDOW;
            styles |= WS_POPUP;
            ::SetWindowLong(m_hwnd, GWL_STYLE, styles);

            // force the window to exactly match the monitor's dimensions and update the frame
            ::SetWindowPos(m_hwnd, HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }

    m_fullscreen = !m_fullscreen;
}

void MainRoutine_Win32::calc_current_monitor_refresh_rate() noexcept
{
    if (nullptr == m_hwnd)
        return;

    const auto fallback = [this]()
    {
        m_frame_budget = 1000.0f / 60.0f;
        LogS("Failed to calc current monitor refresh rate, err " << LogApiERR << ". Reset to default 60Hz and default frame budget " << m_frame_budget << "ms. Warning " << get_frame_budget_warn() << ", critical " << get_frame_budget_crit());
    };

    const auto calculated = [this](DWORD monitor_freq)
    {
        m_monitor_frequency = cast::to_u32(monitor_freq);
        m_frame_budget = cast::to_float(1000.0f / monitor_freq);
        LogS("Monitor refresh rate: " << cast::to_u32(monitor_freq) << "Hz. Frame budget set to " << m_frame_budget << "ms. Warning " << get_frame_budget_warn() << ", critical " << get_frame_budget_crit());
    };

    // get monitor handle where the window is mostly located
    HMONITOR h_monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    if (nullptr == h_monitor)
        return fallback();
    if (h_monitor == m_last_monitor)
        return;
    m_last_monitor = h_monitor;

    DEVMODE dev_mode = {};
    dev_mode.dmSize = sizeof(DEVMODE);
    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode))
    {
        m_monitor_x = cast::to_u32(dev_mode.dmPelsWidth);
        m_monitor_y = cast::to_u32(dev_mode.dmPelsHeight);
    }

    // get monitor info to retrieve the device name
    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(MONITORINFOEXW);
    if (!GetMonitorInfoW(h_monitor, (MONITORINFO*)&mi))
        return fallback();

    // query current display settings
    DEVMODEW dm = {};
    dm.dmSize = sizeof(DEVMODEW);
    dm.dmDriverExtra = 0;
    if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm))
    {
        if (dm.dmFields & DM_DISPLAYFREQUENCY)
            return calculated(dm.dmDisplayFrequency);
    }

    fallback();
}

} // namespace platform

#endif // #ifdef _WIN32
