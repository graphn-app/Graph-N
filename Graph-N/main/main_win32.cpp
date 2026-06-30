
#ifdef _WIN32
#include "main/main_win32.h"
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

MainRoutine_Win32::MainRoutine_Win32()
    : m_d3d_device(nullptr, [](ID3D11Device* p) { if (p) p->Release(); })
    , m_d3d_device_context(nullptr, [](ID3D11DeviceContext* p) { if (p) p->Release(); })
    , m_swap_chain(nullptr, [](IDXGISwapChain* p) { if (p) p->Release(); })
    , m_main_render_target_view(nullptr, [](ID3D11RenderTargetView* p) { if (p) p->Release(); })
    , m_hicon(nullptr, ::DestroyIcon)
    , m_clear_color(0.45f, 0.55f, 0.60f, 1.00f)
{}

MainRoutine_Win32::~MainRoutine_Win32() = default;

bool MainRoutine_Win32::init()
{
    tools::set_exception_translator();

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        LogError(ExcepCategory::API, "Failed to initialize COM, err: " << LogHRES(hr));
        return false;
    }
    hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
    if (FAILED(hr))
    {
        LogError(ExcepCategory::API, "Failed to initialize security, err: " << LogHRES(hr));
        return false;
    }

    m_wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, MainRoutine_Win32::wnd_proc, 0L, 0L,
        ::GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, APP_CAPTION_W, nullptr };
    m_wc.hbrBackground = NULL;
    ::RegisterClassExW(&m_wc);

    const int32_t screen_w = ::GetSystemMetrics(SM_CXSCREEN);
    const int32_t screen_y = ::GetSystemMetrics(SM_CYSCREEN);

    const auto initial_width = 1024;
    const auto initial_height = 768;
    const auto initial_left = screen_w / 2 - initial_width / 2;
    const auto initial_top = screen_y / 2 - initial_height / 2;

    m_hwnd = ::CreateWindowW(m_wc.lpszClassName, APP_CAPTION_W, WS_OVERLAPPEDWINDOW | WS_MAXIMIZE,
        initial_left, initial_top, initial_width, initial_height, nullptr, nullptr,
        m_wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!create_device_d3d(m_hwnd))
    {
        LogError(ExcepCategory::DirectX, "Create_device_d3d failed with err: " << LogERR);
        cleanup_device_d3d();
        ::UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
        return false;
    }

    m_hicon.reset((HICON)::LoadImage(m_wc.hInstance, MAKEINTRESOURCE(101), IMAGE_ICON, 32, 32, LR_LOADTRANSPARENT));
    ::SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hicon.get());
    ::SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hicon.get());

    ::ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
    ::UpdateWindow(m_hwnd);

    ::SetTimer(m_hwnd, 1, 1000, nullptr);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;		// Enable Keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;			// Enabled docking
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = APP_CAPTION_LOW ".ini";
    io.LogFilename = nullptr;
    m_ini_config_exists = tools::path::exists(fs::get_root_directory_path() / io.IniFilename);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX11_Init(m_d3d_device.get(), m_d3d_device_context.get());

    // imguid dialogs
    init_imgui_diloag_texture();

    // font and icons
    prepare_fonts();

    // app obj
    ::SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    return true;
}

int32_t MainRoutine_Win32::run(flow_callback& callback, std::atomic_bool& request_focus)
{
    // Main loop
    m_callback = &callback;
    bool done = false;
    while (!done)
    {
        // wait for evert or timer, but not longer than 20ms
        ::MsgWaitForMultipleObjectsEx(0, NULL, 20, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

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

        if (!render_frame(done))
            done = true;

        if (request_focus.load(std::memory_order_relaxed))
        {
            wake_up();
            request_focus.store(false, std::memory_order_relaxed);
        }
    }

    m_callback = nullptr;
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

    if (!m_main_render_target_view)
    {
        LogError(ExcepCategory::DirectX, "Render target is null");
        ::PostQuitMessage(1);
        return false;
    }

    m_is_rendering = true; // lock re-entrancy

    const auto start_tick = std::chrono::high_resolution_clock::now();

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool ret = true;
    if (m_callback)
    {
        ret = (*m_callback)(done);  // main Graph-N flow
    }

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

    const auto end_tick = std::chrono::high_resolution_clock::now();
    m_frame_render_time = std::chrono::duration<double, std::milli>(end_tick - start_tick).count();

    const UINT32 vsync = (get_vsync_enabled() && !m_moving ? 1 : 0);    // force vsync off when moving
    const HRESULT hr = m_swap_chain->Present(vsync, 0); 
    if (FAILED(hr))
    {
        LogError(ExcepCategory::DirectX, "Device lost on Present: " << LogHRES(hr));
        ::PostQuitMessage(1);
    }

    m_is_rendering = false; // unlock

    return ret;
}

void MainRoutine_Win32::free()
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

    const UINT createDeviceFlags = 0; // D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT; // | D3D11_CREATE_DEVICE_DEBUG
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

    ComPtr<ID3D11Device> pd3dDevice;
    ComPtr<ID3D11DeviceContext> pd3dDeviceContext;
    ComPtr<IDXGISwapChain> pSwapChain;

    HRESULT res = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, pSwapChain.GetAddressOf(), pd3dDevice.GetAddressOf(), &featureLevel, pd3dDeviceContext.GetAddressOf());
    if (DXGI_ERROR_UNSUPPORTED == res) // Try high-performance WARP software driver if hardware is not available.
        res = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, pSwapChain.GetAddressOf(), pd3dDevice.GetAddressOf(), &featureLevel, pd3dDeviceContext.GetAddressOf());
    if (res != S_OK)
    {
        LogError(ExcepCategory::DirectX, "Failed D3D11CreateDevice with error: " << LogERR);
        return false;
    }

    //ComPtr<ID3D11Multithread> multithread;
    //if (SUCCEEDED(pd3dDevice->QueryInterface(IID_PPV_ARGS(multithread.GetAddressOf()))))
    //{
    //    multithread->SetMultithreadProtected(TRUE);
    //}

    m_d3d_device.reset(pd3dDevice.Detach());
    m_d3d_device_context.reset(pd3dDeviceContext.Detach());
    m_swap_chain.reset(pSwapChain.Detach());

    // only 1 frame in buffer
    ComPtr<IDXGIDevice1> dxgiDevice;
    if (SUCCEEDED(m_d3d_device->QueryInterface(__uuidof(IDXGIDevice1), (void**)dxgiDevice.GetAddressOf())))
    {
        dxgiDevice->SetMaximumFrameLatency(1);
        dxgiDevice.Reset();
    }

    return create_render_target();
}

void MainRoutine_Win32::cleanup_device_d3d()
{
    cleanup_render_target();

    m_swap_chain.reset();
    m_d3d_device_context.reset();
    m_d3d_device.reset();
}

void MainRoutine_Win32::cleanup_render_target()
{
    m_main_render_target_view.reset();
}

bool MainRoutine_Win32::create_render_target()
{
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swap_chain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (hr != S_OK || !backBuffer)
    {
        LogError(ExcepCategory::DirectX, "m_swap_chain->GetBuffer failed: " << LogHRES(hr));
        return false;
    }
    std::unique_ptr<ID3D11Texture2D, std::function <void(ID3D11Texture2D*)>> pBackBuffer(backBuffer.Detach(), [](ID3D11Texture2D* p) { p->Release(); });

    ComPtr<ID3D11RenderTargetView> targetView;
    hr = m_d3d_device->CreateRenderTargetView(pBackBuffer.get(), nullptr, targetView.GetAddressOf());
    if (hr != S_OK || !targetView)
    {
        LogError(ExcepCategory::DirectX, "m_d3d_device->CreateRenderTargetView failed: " << LogHRES(hr));
        return false;
    }
    m_main_render_target_view.reset(targetView.Detach());
    return true;
}

void MainRoutine_Win32::init_imgui_diloag_texture()
{
/*
    ifd::FileDialog::Instance().CreateTexture =
        [this, d3d_device = m_d3d_device.get()](uint8_t* data, int32_t w, int32_t h, char fmt) -> void*
    {
        D3D11_TEXTURE2D_DESC desc;
        ::ZeroMemory(&desc, sizeof(desc));
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = (fmt == 0) ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = data;
        subResource.SysMemPitch = w * 4; 
        subResource.SysMemSlicePitch = 0;

        ID3D11Texture2D* pTexture = nullptr;
        HRESULT hr = d3d_device->CreateTexture2D(&desc, &subResource, &pTexture);
        if (FAILED(hr))
        {
            LogError(ExcepCategory::DirectX, "CreateTexture2D failed: " << LogHRES(hr));
            ::PostQuitMessage(1);
            return nullptr;
        }

        ID3D11ShaderResourceView* pSRV = nullptr;
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;

        hr = d3d_device->CreateShaderResourceView(pTexture, &srvDesc, &pSRV);
        if (FAILED(hr))
        {
            LogError(ExcepCategory::DirectX, "CreateShaderResourceView failed: " << LogHRES(hr));
            ::PostQuitMessage(1);
            return nullptr;
        }

        pTexture->Release();
        return (void*)pSRV;
    };

    ifd::FileDialog::Instance().DeleteTexture =
        [](void* texture)
    {
        if (texture != nullptr)
        {
            ID3D11ShaderResourceView* pSRV = (ID3D11ShaderResourceView*)texture;
            pSRV->Release();
        }
    };
*/
}

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
            MainRoutine_Win32* routine = (MainRoutine_Win32*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (routine != nullptr)
                routine->m_moving = true;
        }
        break;
    case WM_EXITSIZEMOVE:
        {
            MainRoutine_Win32* routine = (MainRoutine_Win32*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (routine != nullptr)
                routine->m_moving = false;
        }
        break;
    case WM_SIZE:
        {
            if (SIZE_MINIMIZED == wParam)
            {
                LogS("Windows is minimized");
                if (g::pGraphN)
                    g::pGraphN->handle_wnd_minimized();
                return 0;
            }

            MainRoutine_Win32* routine = (MainRoutine_Win32*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
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

            if (SIZE_RESTORED == wParam && g::pGraphN)
            {
                LogS("Windows is restored");
                g::pGraphN->handle_wnd_restored();
            }
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        if (SC_CLOSE == wParam && g::pGraphN)
        {
            LogS("Exit by pressing X button");
            g::pGraphN->handle_wnd_on_close();
            return 0;
        }
        break;
    case WM_DESTROY:
        LogS("WM_DESTROY");
        DragAcceptFiles(hWnd, FALSE);
        ::PostQuitMessage(0);
        return 0;
    case WM_TIMER:
        if (g::pGraphN)
            g::pGraphN->handle_1sec_timer(++m_seconds_elapsed);
        break;
    case USER_MESSAGE:
        {
            MainRoutine_Win32* routine = (MainRoutine_Win32*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (routine != nullptr && routine->m_user_msg_callback != nullptr)
            {
                routine->m_user_msg_callback(wParam, lParam);
                routine->m_user_msg_callback = nullptr;
            }
        }
        return 0;
    case WM_DROPFILES:
        {
            HDROP hdrop = (HDROP)wParam;
            UINT file_count = DragQueryFileA(hdrop, 0xFFFFFFFF, NULL, 0);
            std::list<std::filesystem::path> files;

            for (UINT i = 0; i < file_count; i++)
            {
                char file_path[MAX_PATH];
                if (DragQueryFileA(hdrop, i, file_path, MAX_PATH))
                {
                    LogS("Dropped file " << file_path);
                    files.push_back(file_path);
                }
            }

            DragFinish(hdrop);

            MainRoutine_Win32* routine = (MainRoutine_Win32*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (routine != nullptr && routine->m_dnd_callback != nullptr)
            {
                routine->m_dnd_callback(std::move(files));
            }
        }
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void MainRoutine_Win32::wake_up()
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

void MainRoutine_Win32::wnd_restore()
{
    Log("Window restore cmd");
    ::ShowWindow(m_hwnd, SW_RESTORE);
    ::SetForegroundWindow(m_hwnd);
    ::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_INTERNALPAINT);
}

void MainRoutine_Win32::wnd_minimize()
{
    Log("Window minimize cmd");
    ::ShowWindow(m_hwnd, SW_MINIMIZE);
}

void MainRoutine_Win32::wnd_maximize()
{
    Log("Window maximize cmd");
    WINDOWPLACEMENT pl = {};
    ::GetWindowPlacement(m_hwnd, &pl);
    pl.showCmd = SW_SHOWMAXIMIZED;
    ::SetWindowPlacement(m_hwnd, &pl);
}

void MainRoutine_Win32::wnd_show()
{
    Log("Window show cmd");
    ::ShowWindow(m_hwnd, SW_SHOW);
    ::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_INTERNALPAINT);
}

void MainRoutine_Win32::wnd_hide()
{
    Log("Window hide cmd");
    ::ShowWindow(m_hwnd, SW_HIDE);
}

void MainRoutine_Win32::wnd_toggle_fullscreen()
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

} // namespace platform

#endif // #ifdef _WIN32
