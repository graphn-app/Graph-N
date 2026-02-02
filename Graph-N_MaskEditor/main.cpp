#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wrl.h>
#include "WebView2.h"
#include <shlwapi.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include "httplib.h"
#include "nlohmann/json.hpp"
#pragma warning(push, 0)
#include <fmt/xchar.h>
#include <fmt/format.h>
#pragma warning(pop)

#define MASKEDITOR_HTML     L"maskeditor.html"
#define ARG_PIPE_NAME       L"-instance"

struct EditorData
{
    HWND hwnd = nullptr;
    std::wstring caption;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> webview_controller;
    Microsoft::WRL::ComPtr<ICoreWebView2> webview;
    std::filesystem::path curr_directory;

    std::string comfyui_host;
    int32_t comfyui_port = 0;

    std::string image_file;
    std::string image_path;
    std::string mask_file;
    bool sam_enabled = true;

    std::vector<BYTE> image_data;
    std::vector<BYTE> mask_data;

    std::wstring pipe_name;    
    HANDLE hpipe = nullptr;

    EditorData() = default;
    ~EditorData()
    {
        if (hpipe)
            ::CloseHandle(hpipe);
    }
}
g_data;

void get_app_directory()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::filesystem::path path(buffer);
    g_data.curr_directory = path.parent_path();
}

std::vector<BYTE> read_stream(IStream* stream)
{
    if (!stream)
        return {};

    std::vector<BYTE> data;
    STATSTG stat;
    if (SUCCEEDED(stream->Stat(&stat, STATFLAG_NONAME)))
        data.reserve(static_cast<size_t>(stat.cbSize.QuadPart));

    BYTE buffer[4096];
    ULONG bytesRead = 0;
    while (SUCCEEDED(stream->Read(buffer, sizeof(buffer), &bytesRead)) && bytesRead > 0)
    {
        data.insert(data.end(), buffer, buffer + bytesRead);
    }
    return data;
}

std::wstring load_html()
{
    auto path = g_data.curr_directory / MASKEDITOR_HTML;
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
    {
        ::MessageBoxW(g_data.hwnd, MASKEDITOR_HTML " not found", g_data.caption.c_str(), MB_ICONSTOP);
        return {};
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open())
    {
        ::MessageBoxW(g_data.hwnd, L"Failed to read " MASKEDITOR_HTML, g_data.caption.c_str(), MB_ICONSTOP);
        return {};
    }

    const auto file_size = std::filesystem::file_size(path);
    if (0 == file_size)
    {
        ::MessageBoxW(g_data.hwnd, L"Invalid " MASKEDITOR_HTML, g_data.caption.c_str(), MB_ICONSTOP);
        return {};
    }

    std::string utf8;
    utf8.resize(file_size);
    if (stream.read(utf8.data(), file_size).fail())
    {
        ::MessageBoxW(g_data.hwnd, L"Failed to read " MASKEDITOR_HTML, g_data.caption.c_str(), MB_ICONSTOP);
        return {};
    }

    return std::wstring(utf8.begin(), utf8.end());
}

bool get_pipe_name()
{
    int num = 0;
    auto args = ::CommandLineToArgvW(::GetCommandLineW(), &num);
    if (nullptr == args || 0 == num)
        return false;

    for (int i = 0; i < num; i++)
    {
        if (_wcsicmp(args[i], ARG_PIPE_NAME) == 0 && i + 1 < num)
            g_data.pipe_name = args[++i];
    }

    ::LocalFree(args);
    return !g_data.pipe_name.empty();
}

bool load_image()
{
    const std::wstring pipe_name = L"\\\\.\\pipe\\" + g_data.pipe_name;
    if (!WaitNamedPipeW(pipe_name.c_str(), NMPWAIT_USE_DEFAULT_WAIT))
        return false;

    HANDLE hpipe = CreateFileW(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, nullptr);
    if (INVALID_HANDLE_VALUE == hpipe)
        return false;

    g_data.hpipe = hpipe;
    DWORD mode = PIPE_READMODE_MESSAGE;
    ::SetNamedPipeHandleState(hpipe, &mode, nullptr, nullptr);

    // 4-imagename-4-imagedata-4-maskpath-4-1/0

    DWORD read = 0;
    uint32_t size = 0;
    if (!::ReadFile(hpipe, &size, 4, &read, nullptr)) return false;
    if (read != 4 || 0 == size) return false;
    g_data.image_file.reserve(size);
    if (!::ReadFile(hpipe, g_data.image_file.data(), size, &read, nullptr))
        return false;

    if (!::ReadFile(hpipe, &size, 4, &read, nullptr)) return false;
    if (read != 4) return false;
    if (size > 0)
    {
        g_data.mask_file.reserve(size);
        if (!::ReadFile(hpipe, g_data.mask_file.data(), size, &read, nullptr))
            return false;
    }
    if (!::ReadFile(hpipe, &size, 4, &read, nullptr)) return false;
    if (read !=4 || size != 4) return false;
    if (!::ReadFile(hpipe, &size, 4, &read, nullptr)) return false;
    if (read !=4) return false;
    g_data.sam_enabled = (1 == size);

    return true;
}

bool load_image_debug()
{
    g_data.comfyui_host = "127.0.0.1";
    g_data.comfyui_port = 8188;
    g_data.image_path = (g_data.curr_directory / "samdetector.png").string();
    g_data.image_file = "example.png";

    std::error_code ec;
    if (!std::filesystem::exists(g_data.image_path, ec))
        return false;

    std::ifstream stream(g_data.image_path, std::ios::binary);
    if (!stream.is_open())
        return false;

    const auto file_size = std::filesystem::file_size(g_data.image_path);
    if (0 == file_size)
        return false;

    g_data.image_data.resize(file_size);
    if (stream.read((char*)g_data.image_data.data(), file_size).fail())
        return false;

    return true;
}

bool save_image()
{
    if (!g_data.hpipe)
        return false;

    DWORD written = 0;
    uint32_t size = static_cast<uint32_t>(g_data.image_data.size());
    if (!::WriteFile(g_data.hpipe, &size, 4, &written, nullptr) || written != 4)
    {
        ::MessageBoxW(g_data.hwnd, L"Failed to save mask", g_data.caption.c_str(), MB_ICONSTOP);
        return false;
    }

    if (!::WriteFile(g_data.hpipe, g_data.image_data.data(), static_cast<DWORD>(g_data.image_data.size()), &written, nullptr) || g_data.image_data.size() != written)
    {
        ::MessageBoxW(g_data.hwnd, L"Failed to save mask", g_data.caption.c_str(), MB_ICONSTOP);
        return false;
    }

    return true;
}

bool detect(std::vector<BYTE> request_data)
{
    try
    {
        using json = nlohmann::json;

        const nlohmann::json prepare_payload =
        {
            { "sam_model_name", "auto" },
            { "filename", g_data.image_file },
            { "type", "input" },
            { "subfolder", "" }
        };

        httplib::Client cli(g_data.comfyui_host, g_data.comfyui_port);
        auto res = cli.Post("/api/sam/prepare", prepare_payload.dump(), "application/json");
        if (!res || res->status != 200)
            return false;

        json pos_points = json::array();
        json neg_points = json::array();

        const nlohmann::json request_json = json::parse(request_data.begin(), request_data.end());
        auto& points = request_json["points"];
        for (auto& pt : points)
        {
            const float x = pt["x"];
            const float y = pt["y"];
            const int32_t pos = pt["label"];

            if (1 == pos)
                pos_points.push_back({ x, y });
            else
                neg_points.push_back({ x, y });
        }
        const float threshold = request_json["threshold"];

        const nlohmann::json detect_payload =
        {
            { "positive_points", pos_points },
            { "negative_points", neg_points },
            { "threshold", threshold }
        };
        res = cli.Post("/api/sam/detect", detect_payload.dump(), "application/json");
        if (!res || res->status != 200 || res->body.empty())
            return false;

        g_data.mask_data.reserve(res->body.size());
        g_data.mask_data.assign(res->body.begin(), res->body.end());
        return true;
    }
    catch (const std::exception&)
    {        
    }
    return false;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (g_data.webview_controller)
        {
            RECT b;
            GetClientRect(hWnd, &b);
            g_data.webview_controller->put_Bounds(b);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

std::wstring create_headers(bool image)
{
    std::wstring headers = L"";
    if (image)
        headers += L"Content-Type: image/png\r\n";
    else
        headers += L"Content-Type: text/plain\r\n";
    headers +=
        L"Access-Control-Allow-Origin: *\r\n"
        L"Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        L"Access-Control-Allow-Headers: Content-Type\r\n"
        L"\r\n";
    return headers;
}

int32_t main(int32_t, char**)
{
    g_data.caption = L"Graph-N Mask Editor";
    get_app_directory();
    //if (!get_pipe_name() || !load_image())
    //    return 1;
    if (!load_image_debug())
        return 1;

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"GraphN_Shim_Host_ME";
    RegisterClassExW(&wcex);

    g_data.hwnd = CreateWindowW(L"GraphN_Shim_Host_ME", g_data.caption.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 1280, 800, NULL, NULL, wcex.hInstance, NULL);
    if (nullptr == g_data.hwnd)
    {
        ::MessageBoxW(nullptr, L"Failed to create app window", g_data.caption.c_str(), MB_ICONSTOP);
        return 1;
    }
    
    ShowWindow(g_data.hwnd, SW_SHOWNORMAL);
    UpdateWindow(g_data.hwnd);

    const auto data_folder = g_data.curr_directory / L"webview_profile";

    const auto hres = CreateCoreWebView2EnvironmentWithOptions(nullptr, data_folder.c_str(), nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT /*result*/, ICoreWebView2Environment* env) -> HRESULT
    {
        const auto hres = env->CreateCoreWebView2Controller(g_data.hwnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [env](HRESULT /*result*/, ICoreWebView2Controller* controller) -> HRESULT
        {
            g_data.webview_controller = controller;
            auto hres = g_data.webview_controller->get_CoreWebView2(&g_data.webview);
            if (FAILED(hres))
                return hres;

            RECT bounds;
            GetClientRect(g_data.hwnd, &bounds);
            g_data.webview_controller->put_Bounds(bounds);
            
            hres = g_data.webview->AddWebResourceRequestedFilter(L"http://app.local/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
            if (FAILED(hres))
                return hres;

            hres = g_data.webview->add_WebResourceRequested(Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [env](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT
            {
                Microsoft::WRL::ComPtr<ICoreWebView2WebResourceRequest> req;
                auto hres = args->get_Request(&req);
                if (SUCCEEDED(hres))
                {
                    LPWSTR puri;
                    hres = req->get_Uri(&puri);
                    if (SUCCEEDED(hres))
                    {
                        std::wstring uri = puri;
                        CoTaskMemFree(puri);

                        if (uri.find(L"http://app.local/input.png") == 0)
                        {
                            Microsoft::WRL::ComPtr<IStream> stream;
                            stream.Attach(SHCreateMemStream(g_data.image_data.data(), static_cast<UINT>(g_data.image_data.size())));
                            if (stream)
                            {
                                const auto headers = create_headers(true);
                                Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> resp;
                                hres = env->CreateWebResourceResponse(stream.Get(), 200, L"OK", headers.c_str(), &resp);
                                if (SUCCEEDED(hres))
                                    hres = args->put_Response(resp.Get());
                            }
                            return hres;
                        }
                        if (uri.find(L"http://app.local/mask.png") == 0)
                        {
                            if (g_data.mask_data.empty())
                            {
                                const auto headers = create_headers(false);
                                Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> resp;
                                hres = env->CreateWebResourceResponse(nullptr, 404, L"Not found", headers.c_str(), &resp);
                                if (SUCCEEDED(hres))
                                    hres = args->put_Response(resp.Get());
                            }
                            else
                            {
                                Microsoft::WRL::ComPtr<IStream> stream;
                                stream.Attach(SHCreateMemStream(g_data.mask_data.data(), static_cast<UINT>(g_data.mask_data.size())));
                                if (stream)
                                {
                                    const auto headers = create_headers(true);
                                    Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> resp;
                                    hres = env->CreateWebResourceResponse(stream.Get(), 200, L"OK", headers.c_str(), &resp);
                                    if (SUCCEEDED(hres))
                                        hres = args->put_Response(resp.Get());
                                }
                            }
                            return hres;
                        }
                        if (uri.find(L"http://app.local/sam") == 0)
                        {
                            const auto headers = create_headers(false);
                            Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> resp;
                            hres = env->CreateWebResourceResponse(nullptr, g_data.sam_enabled ? 200 : 404, L"OK", headers.c_str(), &resp);
                            if (SUCCEEDED(hres))
                                hres = args->put_Response(resp.Get());
                            return hres;
                        }

                        LPWSTR pmethod;
                        hres = req->get_Method(&pmethod);
                        if (SUCCEEDED(hres))
                        {
                            std::wstring method = pmethod;
                            CoTaskMemFree(pmethod);

                            if (L"OPTIONS" == method)
                            {
                                const auto headers = create_headers(false);
                                Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> resp;
                                hres = env->CreateWebResourceResponse(nullptr, 200, L"OK", headers.c_str(), &resp);
                                if (SUCCEEDED(hres))
                                    hres = args->put_Response(resp.Get());
                                return hres;
                            }

                            if (uri.find(L"http://app.local/save_mask") == 0 && L"POST" == method)
                            {
                                Microsoft::WRL::ComPtr<IStream> content_stream;
                                hres = req->get_Content(&content_stream);

                                if (SUCCEEDED(hres) && content_stream)
                                {
                                    g_data.image_data = read_stream(content_stream.Get());
                                    if (!g_data.image_data.empty())
                                        save_image();
                                }

                                Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> resp;
                                hres = env->CreateWebResourceResponse(nullptr, 200, L"OK", L"", &resp);
                                if (SUCCEEDED(hres))
                                    hres = args->put_Response(resp.Get());
                                return hres;
                            }

                            if (uri.find(L"http://app.local/detect") == 0 && L"POST" == method)
                            {
                                Microsoft::WRL::ComPtr<IStream> content_stream;
                                hres = req->get_Content(&content_stream);

                                if (SUCCEEDED(hres) && content_stream)
                                {
                                    auto json = read_stream(content_stream.Get());
                                    if (!json.empty())
                                    {
                                        if (detect(std::move(json)))
                                        {
                                            Microsoft::WRL::ComPtr<IStream> stream;
                                            stream.Attach(SHCreateMemStream(g_data.mask_data.data(), static_cast<UINT>(g_data.mask_data.size())));
                                            if (stream)
                                            {
                                                const auto headers = create_headers(true);
                                                Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> resp;
                                                hres = env->CreateWebResourceResponse(stream.Get(), 200, L"OK", headers.c_str(), &resp);
                                                if (SUCCEEDED(hres))
                                                    hres = args->put_Response(resp.Get());
                                            }
                                        }
                                        else
                                        {
                                            const auto headers = create_headers(false);
                                            Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> resp;
                                            hres = env->CreateWebResourceResponse(nullptr, 500, L"Fail", headers.c_str(), &resp);
                                            if (SUCCEEDED(hres))
                                                hres = args->put_Response(resp.Get());
                                            return hres;
                                        }
                                    }
                                }
                                return hres;
                            }
                        }
                    }
                }
                return hres;
            }).Get(), nullptr);

            if (FAILED(hres))
                return hres;

            const std::wstring html = load_html();
            hres = g_data.webview->NavigateToString(html.c_str());
            return hres;
        }).Get());
        return hres;

    }).Get());

    if (FAILED(hres))
    {
        ::MessageBoxW(nullptr, fmt::format(L"Failed to create webview environment, err 0x{:0>16X}", hres).c_str(), g_data.caption.c_str(), MB_ICONSTOP);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
