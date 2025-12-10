
#ifdef _WIN32
#include "platform/win32/main_win32.h"
#include "tools/exception_handler.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace platform
{

MainRoutine_Win32::MainRoutine_Win32()
	: m_d3d_device(nullptr, [](ID3D11Device* p) { if (p) p->Release(); })
	, m_d3d_device_context(nullptr, [](ID3D11DeviceContext* p) { if (p) p->Release(); })
	, m_swap_chain(nullptr, [](IDXGISwapChain* p) { if (p) p->Release(); })
	, m_main_render_target_view(nullptr, [](ID3D11RenderTargetView* p) { if (p) p->Release(); })
	, m_hicon(nullptr, ::DestroyIcon)
	, m_clear_color(0.45f, 0.55f, 0.60f, 1.00f)
{}

bool MainRoutine_Win32::init()
{
	tools::set_exception_translator();

	m_wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, MainRoutine_Win32::wnd_proc, 0L, 0L,
		GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, APP_CAPTION_W, nullptr };
	m_wc.hbrBackground = NULL;
	::RegisterClassExW(&m_wc);

	m_hwnd = ::CreateWindowW(m_wc.lpszClassName, APP_CAPTION_W, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr, nullptr, m_wc.hInstance, nullptr);	

	// Initialize Direct3D
	if (!create_device_d3d(m_hwnd))
	{
		LogWarn("Create_device_d3d failed with err: " << LgERR);
		cleanup_device_d3d();
		::UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
		return false;
	}

	m_hicon.reset((HICON)LoadImage(m_wc.hInstance, MAKEINTRESOURCE(101), IMAGE_ICON, 32, 32, LR_LOADTRANSPARENT));
	::SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hicon.get());
	::SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hicon.get());

	::ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
	::UpdateWindow(m_hwnd);

	::SetTimer(m_hwnd, 1, 1000, nullptr);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;		// Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;			// Enabled docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;			// Enable multiple viewports
	io.IniFilename = APP_CAPTION_LOW ".ini";

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX11_Init(m_d3d_device.get(), m_d3d_device_context.get());

	auto font_file = fs::get_res_directory_path();
	font_file /= "verdana.ttf";
	m_font = io.Fonts->AddFontFromFileTTF(font_file.string().c_str(), 18.0f);

	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	return true;
}

int MainRoutine_Win32::run(flow_callback& callback, std::atomic_bool& request_focus)
{
	// Main loop
	m_callback = &callback;
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the wnd_proc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
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

		if (request_focus)
		{
			wake_up();
			request_focus = false;
		}
	}

	m_callback = nullptr;
	return 0;
}

bool MainRoutine_Win32::render_frame(bool done)
{
	if (!m_main_render_target_view)
	{
		LogWarn("Render target is null");
		::PostQuitMessage(1);
		return false;
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//	draw_app_version(ImGui::GetBackgroundDrawList());

	bool ret = true;
	if (m_callback)
	{
		ret = (*m_callback)(done);	// main Graph-N flow
	}

	// Rendering
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

	const HRESULT hr = m_swap_chain->Present(m_moving ? 0 : 1, 0); // don't use vsync when moving
	if (FAILED(hr))
	{
		LogWarn("Device lost on Present: " << LgHEX(hr));
		::PostQuitMessage(1);
	}

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
}

bool MainRoutine_Win32::create_device_d3d(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
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

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

	ID3D11Device* pd3dDevice = nullptr;
	ID3D11DeviceContext* pd3dDeviceContext = nullptr;
	IDXGISwapChain* pSwapChain = nullptr;

	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);
	if (DXGI_ERROR_UNSUPPORTED == res) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);
	if (res != S_OK)
	{
		LogWarn("Failed D3D11CreateDevice with error: " << LgERR);
		return false;
	}
	m_d3d_device.reset(pd3dDevice);
	m_d3d_device_context.reset(pd3dDeviceContext);
	m_swap_chain.reset(pSwapChain);

	// only 1 frame in buffer
	IDXGIDevice1* dxgiDevice = nullptr;
	if (SUCCEEDED(m_d3d_device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice)))
	{
		dxgiDevice->SetMaximumFrameLatency(1);
		dxgiDevice->Release();
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
	ID3D11Texture2D* backBuffer = nullptr;
	HRESULT hr = m_swap_chain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	if (hr != S_OK || nullptr == backBuffer)
	{
		LogWarn("m_swap_chain->GetBuffer failed: " << LgHEX(hr));
		return false;
	}
	std::unique_ptr<ID3D11Texture2D, std::function <void(ID3D11Texture2D*)>> pBackBuffer(backBuffer, [](ID3D11Texture2D* p) { p->Release(); });

	ID3D11RenderTargetView* targetView = nullptr;
	hr = m_d3d_device->CreateRenderTargetView(pBackBuffer.get(), nullptr, &targetView);
	if (hr != S_OK || nullptr == targetView)
	{
		LogWarn("m_d3d_device->CreateRenderTargetView failed: " << LgHEX(hr));
		return false;
	}
	m_main_render_target_view.reset(targetView);
	return true;
}

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI MainRoutine_Win32::wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_ENTERSIZEMOVE:
		{
			MainRoutine_Win32* routine = (MainRoutine_Win32*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (routine != nullptr)
				routine->m_moving = true;
		}
		break;
	case WM_EXITSIZEMOVE:
		{
			MainRoutine_Win32* routine = (MainRoutine_Win32*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (routine != nullptr)
				routine->m_moving = false;
		}
		break;
	case WM_SIZE:
		{
			if (SIZE_MINIMIZED == wParam)
			{
				if (g::pGraphN)
					g::pGraphN->handle_wnd_minimized();
				return 0;
			}

			MainRoutine_Win32* routine = (MainRoutine_Win32*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (routine != nullptr)
			{
				routine->cleanup_render_target();
				routine->m_swap_chain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				if (!routine->create_render_target())
					::PostQuitMessage(1);
				else
					routine->render_frame(false /*done*/);
			}

			if (SIZE_RESTORED == wParam && g::pGraphN)
				g::pGraphN->handle_wnd_restored();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		if (SC_CLOSE == wParam && g::pGraphN)
		{
			Log("Exit by pressing X button");
			g::pGraphN->handle_wnd_on_close();
			return 0;
		}
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	case WM_TIMER:
		if (g::pGraphN)
			g::pGraphN->handle_1sec_timer();
		break;
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
	Log("Window restored");
	::ShowWindow(m_hwnd, SW_RESTORE);
	::SetForegroundWindow(m_hwnd);
	::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_INTERNALPAINT);
}

void MainRoutine_Win32::wnd_minimize()
{
	Log("Window minimized");
	::ShowWindow(m_hwnd, SW_MINIMIZE);
}

void MainRoutine_Win32::wnd_maximize()
{
	Log("Window maximized");
	WINDOWPLACEMENT pl = {};
	GetWindowPlacement(m_hwnd, &pl);
	pl.showCmd = SW_SHOWMAXIMIZED;
	SetWindowPlacement(m_hwnd, &pl);
}

void MainRoutine_Win32::wnd_show()
{
	Log("Window show");
	::ShowWindow(m_hwnd, SW_SHOW);
	::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_INTERNALPAINT);
}

void MainRoutine_Win32::wnd_hide()
{
	Log("Window hide");
	::ShowWindow(m_hwnd, SW_HIDE);
}

} // namespace platform

#endif // #ifdef _WIN32