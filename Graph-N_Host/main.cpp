/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#define MASKEDITOR_HTML         "mask_editor.html"
#define POSEDITOR_HTML          "3dpos_editor.html"

#define IDI_POSEDITOR           102
#define IDI_MASKEDITOR          103

#define APPCAPT_POSEDITOR       "Graph-N 3DPos Editor"
#define APPCAPT_POSEDITOR_W     L"Graph-N 3DPos Editor"
#define APPCAPT_MASKEDITOR      "Graph-N Mask Editor"
#define APPCAPT_MASKEDITOR_W    L"Graph-N Mask Editor"

#define ARG_PIPE_NAME           L"-pipe"
#define ARG_ID                  L"-id"
#define ARG_DEBUG_MSK           L"-debug-mask-editor"
#define ARG_DEBUG_3DP           L"-debug-3dpos-editor"
#define ARG_SRC_IMAGE           L"-src"     // debug mode
#define ARG_DEST_IMAGE          L"-dest"    // debug mode
#define ARG_CATCH               L"-catch"

#define USER_MSG_FOREGROUND     (WM_APP + 1) // bring window to foreground
#define USER_MSG_RENAME         (WM_APP + 2) // set new caption
#define USER_MSG_INVOKE         (WM_APP + 3) // for UI thread dispatching

using namespace std::chrono_literals;

enum class Mode
{
    Unknown,
    MaskEditor,
    PosEditor
};

using Handle = std::unique_ptr<void, decltype(&::CloseHandle)>;

struct EditorData final
{
    // window
    HWND hwnd = nullptr;

    // app
    bool debug_mode = false;
    Mode mode = Mode::Unknown;

    // webview
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> webview_controller;
    Microsoft::WRL::ComPtr<ICoreWebView2> webview;

    // binary
    std::string src_data;
    std::string dest_data;
    std::string mask_data;
    std::wstring html_data;

    // pipe
    std::wstring pipe_name;
    Handle hpipe;
    std::atomic_bool image_ready = false;

    // server data
    blob::HostData host_data;

    EditorData() noexcept
        : hpipe(nullptr, ::CloseHandle)
    {}
    ~EditorData() noexcept = default;
}
g_data;

namespace
{

bool read_args()
{
    int32_t num = 0;
    const auto args = ::CommandLineToArgvW(::GetCommandLineW(), &num);
    if (nullptr == args || 0 == num)
        return false;

    for (int32_t i = 0; i < num; i++)
    {
        if (std::wcscmp(args[i], ARG_PIPE_NAME) == 0 && i + 1 < num)
        {
            g_data.pipe_name = args[++i];
        }
        else if (std::wcscmp(args[i], ARG_ID) == 0 && i + 1 < num)
        {
            g_data.host_data.host_id = std::wcstoull(args[++i], nullptr, 10);
        }

        else if (std::wcscmp(args[i], ARG_DEBUG_MSK) == 0)
        {
            g_data.debug_mode = true;
            g_data.mode = Mode::MaskEditor;
        }
        else if (std::wcscmp(args[i], ARG_DEBUG_3DP) == 0)
        {
            g_data.debug_mode = true;
            g_data.mode = Mode::PosEditor;
        }
        else if (std::wcscmp(args[i], ARG_SRC_IMAGE) == 0 && i + 1 < num)
        {
            g_data.host_data.source_path = args[++i];
        }
        else if (std::wcscmp(args[i], ARG_DEST_IMAGE) == 0 && i + 1 < num)
        {
            g_data.host_data.dest_path = args[++i];
        }
        else if (std::wcscmp(args[i], ARG_CATCH) == 0)
        {
            DEBUG_CATCH;
        }
    }

    ::LocalFree(args);

    if (g_data.debug_mode)
    {
        LogRAssertS(Mode::MaskEditor == g_data.mode || Mode::PosEditor == g_data.mode, "Wrong debug mode", false);
        LogRAssertS(tools::path::exists(g_data.host_data.source_path), "Source path empty or not exists", false);
        LogRAssertS(tools::path::exists(g_data.host_data.dest_path), "Dest path empty or not exists", false);
        LogRAssertS(g_data.pipe_name.empty(), "Pipe cannot be used in debug mode", false);
        LogRAssertS(0 == g_data.host_data.host_id, "Host id cannot be used in debug mode", false);
        g_data.host_data.settings = R"({"debug":true})";
        if (Mode::MaskEditor == g_data.mode) LogS("Starting MaskEditor in debug mode");
        else LogS("Starting 3DPosEditor in debug mode");
    }
    else
    {
        LogRAssertS(!g_data.pipe_name.empty(), "Pipe name cannot be empty", false);
        LogRAssertS(g_data.host_data.host_id != 0, "Host id must be set", false);
        LogS("Host #" << g_data.host_data.host_id << ": started");
    }

    return true;
}

void listen_thread(std::promise<bool> promise)
{
    const auto set_reply = [&promise](bool result, const char* error = nullptr)
    {
        if (!result) LogS(error << ", api-code: " << LogApiERR);
        promise.set_value(result);
    };

    const std::wstring pipe_name = L"\\\\.\\pipe\\" + g_data.pipe_name;
    if (!::WaitNamedPipeW(pipe_name.c_str(), NMPWAIT_USE_DEFAULT_WAIT))
        return set_reply(false, "No such pipe");

    const HANDLE hpipe = ::CreateFileW(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (INVALID_HANDLE_VALUE == hpipe)
        return set_reply(false, "Can't open pipe");
    LogS("Pipe open: " << pipe_name);

    g_data.hpipe.reset(hpipe);
    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!::SetNamedPipeHandleState(hpipe, &mode, nullptr, nullptr))
        return set_reply(false, "Can't set pipe state");

    const auto write_pipe = [](const blob::HostData& data)
    {
        DWORD bytes_written = 0;
        ::OVERLAPPED write_ov = {};
        write_ov.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

        bool result = false;
        const auto blob = blob::HostSerializer::serialize(data);
        if (!::WriteFile(g_data.hpipe.get(), blob->data(), cast::to<DWORD>(blob->size()), &bytes_written, &write_ov))
        {
            if (::GetLastError() == ERROR_IO_PENDING)
            {
                if (::GetOverlappedResult(g_data.hpipe.get(), &write_ov, &bytes_written, TRUE))
                {
                    LogS("Pipe: sent " << bytes_written << " bytes");
                    result = true;
                }
                else
                    LogWarnS("Can't write to pipe - overlapped result failed. err: " << LogApiERR);
            }
            else
                LogWarnS("Can't write to pipe. err: " << LogApiERR);
        }
        else
        {
            LogS("Pipe: sent " << bytes_written << " bytes");
            result = true;
        }

        ::CloseHandle(write_ov.hEvent);
        return result;
    };

    const auto min_size = cast::to<DWORD>(blob::HostSerializer::get_min_size());
    const auto get_data_ready = [min_size]() -> DWORD
    {
        DWORD bytes_avail = 0;
        const BOOL peek_status = ::PeekNamedPipe(g_data.hpipe.get(), nullptr, 0, nullptr, nullptr, &bytes_avail);
        if (peek_status && bytes_avail >= min_size)
        {
            LogS("Message ready: " << bytes_avail << " bytes");
            return bytes_avail;
        }
        return 0;
    };

    const auto read_pipe = [](DWORD data_size, blob::HostData& data)
    {
        byte_buff buffer(data_size);
        DWORD bytes_read = 0;
        ::OVERLAPPED read_ov = {};
        read_ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

        bool success = false;
        if (::ReadFile(g_data.hpipe.get(), buffer.data(), data_size, &bytes_read, &read_ov))
        {
            LogS("Pipe: read " << bytes_read << " bytes");
            success = true;
        }
        else if (::GetLastError() == ERROR_IO_PENDING)
        {
            if (::GetOverlappedResult(g_data.hpipe.get(), &read_ov, &bytes_read, TRUE))
            {
                LogS("Pipe: read " << bytes_read << " bytes");
                success = true;
            }
            else
                LogWarnS("Readfile overlapped result err : " << LogApiERR);
        }

        ::CloseHandle(read_ov.hEvent);

        if (!success || bytes_read != data_size)
            return false;

        blob::Blob blob(std::move(buffer));
        data = blob::HostSerializer::deserialize(blob);

        if (data.host_id != g_data.host_data.host_id)
            LogWarnS("Unknown host id in command: " << data.host_id);
        if (blob::HostData::Cmd_None == data.cmd)
            LogWarnS("Unknown command: " << cast::to_u32(data.cmd));
        return data.cmd != blob::HostData::Cmd_None && data.host_id == g_data.host_data.host_id;
    };

    {
        blob::HostData ready_data;
        ready_data.host_id = g_data.host_data.host_id;
        ready_data.cmd = blob::HostData::Cmd_Ready;
        LogS("Send: ready");
        if (!write_pipe(ready_data))
            return set_reply(false, "Can't write init data");
    }

    DWORD bytes_ready = 0;
    while (0 == bytes_ready)
    {
        bytes_ready = get_data_ready();
        if (0 == bytes_ready)
            std::this_thread::sleep_for(100ms);
    }

    blob::HostData read_data;
    if (!read_pipe(bytes_ready, read_data))
        return set_reply(false, "Can't read init data");
    if (0 == read_data.host_id || read_data.host_id != g_data.host_data.host_id)
        return set_reply(false, "Invalid host id read");

    g_data.host_data = std::move(read_data);
    if (g_data.host_data.cmd != blob::HostData::Cmd_MaskEditor && g_data.host_data.cmd != blob::HostData::Cmd_PosEditor)
        return set_reply(false, "Invalid mode");
    g_data.mode = (blob::HostData::Cmd_MaskEditor == g_data.host_data.cmd ? Mode::MaskEditor : Mode::PosEditor);
    LogS("Mode: " << (Mode::MaskEditor == g_data.mode ? "Massk editor" : "3dpos editor"));
    if (0 == g_data.host_data.host_id)
        return set_reply(false, "Invalid host id");
    if (!tools::path::exists(g_data.host_data.source_path))
        return set_reply(false, "Source path is not exists");
    LogS("Source: " << g_data.host_data.source_path);
    if (!tools::path::exists(g_data.host_data.dest_path))
        return set_reply(false, "Dest path is not exists");
    LogS("Dest: " << g_data.host_data.dest_path);
    if (0 == g_data.host_data.node_id || 0 == g_data.host_data.workflow_id)
        return set_reply(false, "Invalid node/workflow id");    
    if (g_data.host_data.wf_name.empty() || g_data.host_data.node_name.empty())
        return set_reply(false, "Empty node/workflow name");
    LogS("Workflow. " << g_data.host_data.workflow_id << ":" << g_data.host_data.wf_name << ", Node. " << g_data.host_data.node_id << ":" << g_data.host_data.node_name);
    if (blob::HostData::Cmd_MaskEditor == g_data.host_data.cmd && g_data.host_data.sam_enabled && (g_data.host_data.comfyui_host.empty() || 0 == g_data.host_data.comfyui_port))
        return set_reply(false, "SAM enabled, but ComfyUI server not specified");
    LogS("SAM enabled: " << g_data.host_data.sam_enabled << ", ComfyUI at: " << g_data.host_data.comfyui_host << ":" << g_data.host_data.comfyui_port);

    set_reply(true, nullptr);
    LogS("Host initialized");

    while (true)
    {
        bytes_ready = get_data_ready();
        if (bytes_ready > 0)
        {
            read_data = blob::HostData{};
            if (read_pipe(bytes_ready, read_data))
            {
                switch (read_data.cmd)
                {
                case blob::HostData::Cmd_Raise:
                    LogS("Raise command received");
                    ::PostMessage(g_data.hwnd, USER_MSG_FOREGROUND, 0, 0);
                    break;
                case blob::HostData::Cmd_Rename:
                    {
                        LogS("Rename command received: w:" << read_data.wf_name << ", n:" << read_data.node_name);
                        char* wf_buff = new char[read_data.wf_name.size() + 2];
                        strcpy_s(wf_buff, read_data.wf_name.size() + 1, read_data.wf_name.c_str());
                        char* node_buff = new char[read_data.node_name.size() + 2];
                        strcpy_s(node_buff, read_data.node_name.size() + 1, read_data.node_name.c_str());
                        ::PostMessage(g_data.hwnd, USER_MSG_RENAME, cast::from_ptr<WPARAM>(wf_buff), cast::from_ptr<LPARAM>(node_buff));
                    }
                    break;
                case blob::HostData::Cmd_Terminate:
                    LogS("Terminate command received");
                    ::PostMessage(g_data.hwnd, WM_DESTROY, 0, 0);
                    return;
                case blob::HostData::Cmd_Shutdown:
                    LogS("Shutdown command received");
                    ::PostMessage(g_data.hwnd, WM_DESTROY, 0, 0);
                    return;
                default:
                    LogErrorS("Invalid command received: " << read_data.cmd);
                }
            }
            else
            {
                LogS("Pipe data ready(" << bytes_ready << " bytes), but reading failed. api code: " << LogApiERR);
            }
        }
        else if (g_data.image_ready.load(std::memory_order_relaxed))
        {
            blob::HostData apply_data;
            apply_data.host_id = g_data.host_data.host_id;
            apply_data.cmd = blob::HostData::Cmd_Apply;
            apply_data.settings = g_data.host_data.settings;
            LogS("Send: apply");
            write_pipe(apply_data);
        }

        std::this_thread::sleep_for(100ms);
    }
}

void build_caption()
{
    const char* editor = (Mode::MaskEditor == g_data.mode ? APPCAPT_MASKEDITOR : APPCAPT_POSEDITOR);
    const auto caption = fmt::format(FMT_STRING("{}. {}: {}"), editor, g_data.host_data.wf_name, g_data.host_data.node_name);
    if (g_data.hwnd != nullptr)
        ::SetWindowTextA(g_data.hwnd, caption.c_str());
    LogS("Caption rebuilt: " << caption);
}

bool connect_pipe()
{
    if (g_data.debug_mode)
        return true;

    std::promise<bool> promise;
    auto future = promise.get_future();

#ifdef _DEBUG
    constexpr auto pipe_timeout = 500s;
#else
    constexpr auto pipe_timeout = 5s;
#endif

    std::thread(listen_thread, std::move(promise)).detach();
    if (future.wait_for(pipe_timeout) == std::future_status::timeout)
    {
        LogS("Pipe is not ready. Exit by timeout");
        return false;
    }

    if (false == future.get())
    {
        LogS("Pipe is not ready. Exit by result");
        return false;
    }

    return true;
}

bool load_html()
{
    const char* html_name = (Mode::MaskEditor == g_data.mode ? MASKEDITOR_HTML : POSEDITOR_HTML);
    const auto path = fs::get_root_directory_path() / html_name;
    if (!tools::path::exists(path))
    {
        LogS("HTML file not found: " << path);
        return false;
    }

    auto html = tools::path::read_file(path, false);
    if (html.empty())
    {
        LogS("Failed to read HTML file: " << path);
        return false;
    }
    LogS("HTML loaded from: " << path);

    if (g_data.host_data.settings.empty())
        tools::str::replace_this(html, "/*APP_SETTINGS_TOKEN*/", "null");
    else
        tools::str::replace_this(html, "/*APP_SETTINGS_TOKEN*/", g_data.host_data.settings);
    g_data.html_data.assign(html.begin(), html.end());
    return true;
}

bool load_source_image()
{
    if (!tools::path::exists(g_data.host_data.source_path))
    {
        LogS("Image file not found: " << g_data.host_data.source_path);
        return false;
    }

    const auto image = tools::path::read_file(g_data.host_data.source_path, false);
    if (image.empty())
    {
        LogS("Failed to read Image file: " << g_data.host_data.source_path);
        return false;
    }

    LogS("Image loaded from: " << g_data.host_data.source_path);
    g_data.src_data = std::move(image);
    return true;
}

bool save_dest_image() noexcept
{
    if (!tools::path::exists(g_data.host_data.dest_path))
    {
        LogS("Dest Image file not found: " << g_data.host_data.dest_path);
        return false;
    }

    if (!tools::path::make_file(g_data.host_data.dest_path, true /*overwrite*/, g_data.dest_data))
    {
        LogS("Failed to write Image file: " << g_data.host_data.dest_path);
        return false;
    }

    LogS("Image saved to: " << g_data.host_data.dest_path);
    return true;
}

std::string read_stream(::IStream* stream)
{
    if (nullptr == stream)
        return {};

    std::string data;
    ::STATSTG stat;
    if (SUCCEEDED(stream->Stat(&stat, STATFLAG_NONAME)))
        data.reserve(cast::to_szt(stat.cbSize.QuadPart));

    byte_buff buffer(4096);
    ULONG bytesRead = 0;
    while (SUCCEEDED(stream->Read(buffer.data(), cast::to<ULONG>(buffer.size()), &bytesRead)) && bytesRead > 0)
    {
        data.insert(data.end(), buffer.data(), buffer.data() + bytesRead);
    }
    return data;
}

std::string sam_detect(const std::string& request_data)
{
    try
    {
        using json = nlohmann::json;

        const nlohmann::json prepare_payload =
        {
            { "sam_model_name", "auto" },
            { "filename", g_data.host_data.source_path },
            { "type", "input" },
            { "subfolder", "" }
        };

        httplib::Client cli(g_data.host_data.comfyui_host, g_data.host_data.comfyui_port);
        auto res = cli.Post("/api/sam/prepare", prepare_payload.dump(), "application/json");
        if (!res || res->status != 200)
        {
            LogS("/api/sam/prepare returned code " << res->status);
            return {};
        }

        json pos_points = json::array();
        json neg_points = json::array();

        const nlohmann::json request_json = json::parse(request_data);
        auto& points = request_json["points"];
        std::stringstream pt_str_pos, pt_str_neg;
        for (auto& pt : points)
        {
            const float x = pt["x"];
            const float y = pt["y"];
            const int32_t pos = pt["label"];

            if (1 == pos)
            {
                pos_points.push_back({ x, y });
                if (!pt_str_pos.str().empty())
                    pt_str_pos << ", ";
                pt_str_pos << x << "/" << y;
            }
            else
            {
                neg_points.push_back({ x, y });
                if (!pt_str_neg.str().empty())
                    pt_str_neg << ", ";
                pt_str_neg << x << "/" << y;
            }
        }
        LogS("SAM points. Pos: " << pt_str_pos.str() << ", Neg: " << pt_str_neg.str());

        const float threshold = request_json["threshold"];
        const nlohmann::json detect_payload =
        {
            { "positive_points", pos_points },
            { "negative_points", neg_points },
            { "threshold", threshold }
        };
        res = cli.Post("/api/sam/detect", detect_payload.dump(), "application/json");
        if (!res || res->status != 200 || res->body.empty())
        {
            LogS("/api/sam/detect returned code " << res->status << " and body size " << res->body.size());
            return {};
        }

        return std::move(res->body);
    }
    catch (const std::exception& ex)
    {
        LogExcepS("SAM detect: " << ex.what());
    }
    return {};
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (g_data.webview_controller)
        {
            RECT b;
            ::GetClientRect(hWnd, &b);
            g_data.webview_controller->put_Bounds(b);
        }
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    case USER_MSG_FOREGROUND:
        if (::IsIconic(g_data.hwnd))
        {
            ::ShowWindow(g_data.hwnd, SW_RESTORE);
            ::SetForegroundWindow(g_data.hwnd);
            ::RedrawWindow(g_data.hwnd, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_INTERNALPAINT);
        }
        else
        {
            ::BringWindowToTop(g_data.hwnd);
            ::SetForegroundWindow(g_data.hwnd);
        }
        break;
    case USER_MSG_RENAME:
        {
            char* wf_name = cast::ptrs<char>(wParam);
            char* node_name = cast::ptrs<char>(lParam);
            if (wf_name != nullptr)
            {
                g_data.host_data.wf_name = wf_name;
                delete[] wf_name;
            }
            if (node_name != nullptr)
            {
                g_data.host_data.node_name = node_name;
                delete[] node_name;
            }
            build_caption();
        }
        break;
    case USER_MSG_INVOKE:
        {
            auto* task = cast::ptrs<std::function<void()>>(lParam);
            if (task != nullptr)
            {
                (*task)();
                delete task;
            }
        }
        break;
    default:
        return ::DefWindowProcW(hWnd, message, wParam, lParam);
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
        L"Access-Control-Allow-Headers: Content-Type, X-Editor-Settings\r\n"
        L"\r\n"
    ;
    return headers;
}

} // namespace

int32_t main(int32_t, char**)
{
    tools::dt::save_app_start_time();
    glog::LogGuard logger;

    if (!read_args()) return 1;
    if (!connect_pipe()) return 1;
    if (!load_html()) return 1;
    if (!load_source_image()) return 1;

    ::WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(::WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"GraphN_Shim_Host_ME";
    ::RegisterClassExW(&wcex);

    const wchar_t* editor = (Mode::MaskEditor == g_data.mode ? APPCAPT_MASKEDITOR_W : APPCAPT_POSEDITOR_W);
    g_data.hwnd = ::CreateWindowW(L"GraphN_Shim_Host_ME", editor, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 1280, 800, nullptr, nullptr, wcex.hInstance, nullptr);
    if (nullptr == g_data.hwnd)
    {
        LogS("Failed to create app window. api code: " << LogApiERR);
        return 1;
    }

    std::unique_ptr<HICON__, decltype(&::DestroyIcon)> hicon(nullptr, ::DestroyIcon);
    if (Mode::MaskEditor == g_data.mode)
        hicon.reset((HICON)::LoadImage(wcex.hInstance, MAKEINTRESOURCE(IDI_MASKEDITOR), IMAGE_ICON, 32, 32, LR_LOADTRANSPARENT));
    else
        hicon.reset((HICON)::LoadImage(wcex.hInstance, MAKEINTRESOURCE(IDI_POSEDITOR), IMAGE_ICON, 32, 32, LR_LOADTRANSPARENT));
    ::SendMessage(g_data.hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon.get());
    ::SendMessage(g_data.hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon.get());

    BOOL dark = TRUE;
    ::DwmSetWindowAttribute(g_data.hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));

    ::ShowWindow(g_data.hwnd, SW_SHOWMAXIMIZED);
    ::UpdateWindow(g_data.hwnd);

    build_caption();

    const auto send_response = [](bool image_headers, ::IStream* stream,
        ::ICoreWebView2Environment* env, ::ICoreWebView2WebResourceRequestedEventArgs* args,
        int32_t status_code, const wchar_t* reason,
        const char* details)
    {
        const auto headers = create_headers(image_headers);
        ::Microsoft::WRL::ComPtr<::ICoreWebView2WebResourceResponse> resp;
        auto hres = env->CreateWebResourceResponse(stream, status_code, reason, headers.c_str(), &resp);
        if (SUCCEEDED(hres))
        {
            hres = args->put_Response(resp.Get());
            if (FAILED(hres))
                LogS("ICoreWebView2WebResourceRequestedEventArgs::put_Response(" << details << ") failed. HR: " << LogHRES(hres));
            else
                LogS("Respond sent: " << status_code << ", " << reason << ", " << details);
        }
        else
            LogS("ICoreWebView2Environment::CreateWebResourceResponse(" << details << ") failed. HR: " << LogHRES(hres));
        return hres;
    };

    const auto data_folder = fs::get_host_webprofile_path(g_data.host_data.host_id);
    {
        const auto hres = ::CreateCoreWebView2EnvironmentWithOptions(nullptr, data_folder.c_str(), nullptr,
            ::Microsoft::WRL::Callback<::ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [send_response](HRESULT hres, ::ICoreWebView2Environment* env) -> HRESULT
        {
            if (nullptr == env || FAILED(hres))
            {
                LogS("CreateCoreWebView2EnvironmentWithOptions failed. HR: " << LogHRES(hres));
                return hres;
            }
            hres = env->CreateCoreWebView2Controller(g_data.hwnd, Microsoft::WRL::Callback<::ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [env, send_response](HRESULT hres, ::ICoreWebView2Controller* controller) -> HRESULT
            {
                if (nullptr == controller || FAILED(hres))
                {
                    LogS("ICoreWebView2Environment::CreateCoreWebView2Controller failed. HR: " << LogHRES(hres));
                    return hres;
                }
                g_data.webview_controller = controller;
                hres = g_data.webview_controller->get_CoreWebView2(&g_data.webview);
                if (FAILED(hres))
                {
                    LogS("ICoreWebView2Controller::get_CoreWebView2 failed. HR: " << LogHRES(hres));
                    return hres;
                }

                ::RECT bounds = {};
                ::GetClientRect(g_data.hwnd, &bounds);
                hres = g_data.webview_controller->put_Bounds(bounds);
                LogS("Bounds set: " << int32_t(bounds.left) << "/" << int32_t(bounds.top) << ", " << int32_t(bounds.right) << "/" << int32_t(bounds.bottom) << ", res: " << LogHRES(hres));

                hres = g_data.webview->AddWebResourceRequestedFilter(L"http://app.local/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
                if (FAILED(hres))
                {
                    LogS("ICoreWebView2::AddWebResourceRequestedFilter failed. HR: " << LogHRES(hres));
                    return hres;
                }

                hres = g_data.webview->add_WebResourceRequested(::Microsoft::WRL::Callback<::ICoreWebView2WebResourceRequestedEventHandler>(
                    [env, send_response](::ICoreWebView2*, ::ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT
                {
                    if (nullptr == args)
                    {
                        LogS("ICoreWebView2::add_WebResourceRequested failed: zero args");
                        return E_INVALIDARG;
                    }
                    ::Microsoft::WRL::ComPtr<::ICoreWebView2WebResourceRequest> req;
                    auto hres = args->get_Request(&req);
                    if (FAILED(hres))
                    {
                        LogS("ICoreWebView2WebResourceRequestedEventArgs::get_Request failed. HR: " << LogHRES(hres));
                        return hres;
                    }

                    LPWSTR puri;
                    hres = req->get_Uri(&puri);
                    if (FAILED(hres))
                    {
                        LogS("ICoreWebView2WebResourceRequest::get_Uri failed. HR: " << LogHRES(hres));
                        return hres;
                    }


                    std::wstring uri = puri;
                    ::CoTaskMemFree(puri);

                    LogS("URL requested: " << uri);
                    if (uri.starts_with(L"http://app.local/input.png"))
                    {
                        ::Microsoft::WRL::ComPtr<::IStream> stream;
                        stream.Attach(::SHCreateMemStream(cast::ptrs<byte>(g_data.src_data.data()), cast::to<UINT>(g_data.src_data.size())));
                        if (stream)
                            hres = send_response(true, stream.Get(), env, args, 200, L"OK", "input.png");
                        else
                        {
                            LogS("Failed to attach stream (input.png)");
                            hres = E_FAIL;
                        }
                        return hres;
                    }

                    if (uri.starts_with(L"http://app.local/mask.png"))
                    {
                        if (g_data.mask_data.empty())
                            hres = send_response(false, nullptr, env, args, 404, L"Not found", "mask.png 404");
                        else
                        {
                            ::Microsoft::WRL::ComPtr<::IStream> stream;
                            stream.Attach(::SHCreateMemStream(cast::ptrs<byte>(g_data.mask_data.data()), cast::to<UINT>(g_data.mask_data.size())));
                            if (stream)
                                hres = send_response(true, stream.Get(), env, args, 200, L"OK", "mask.png 200");
                            else
                            {
                                LogS("Failed to attach stream (mask.png 200)");
                                hres = E_FAIL;
                            }
                        }
                        return hres;
                    }
                    if (uri.starts_with(L"http://app.local/sam"))
                    {
                        hres = send_response(false, nullptr, env, args, g_data.host_data.sam_enabled ? 200 : 404, L"OK", "SAM");
                        return hres;
                    }

                    LPWSTR pmethod = nullptr;
                    hres = req->get_Method(&pmethod);
                    if (FAILED(hres))
                    {
                        LogS("ICoreWebView2WebResourceRequest::get_Method failed. HR: " << LogHRES(hres));
                        return hres;
                    }

                    const std::wstring method = pmethod;
                    ::CoTaskMemFree(pmethod);

                    LogS("Method requested: " << method);
                    if (L"OPTIONS" == method)
                    {
                        hres = send_response(false, nullptr, env, args, 200, L"OK", "OPTIONS");
                        return hres;
                    }

                    if (uri.starts_with(L"http://app.local/save_mask") && L"POST" == method)
                    {
                        ::Microsoft::WRL::ComPtr<::ICoreWebView2HttpRequestHeaders> headers;
                        g_data.host_data.settings.clear();
                        if (SUCCEEDED(req->get_Headers(&headers)) && headers)
                        {
                            LPWSTR header_value = nullptr;
                            if (SUCCEEDED(headers->GetHeader(L"X-Editor-Settings", &header_value)) && header_value)
                            {
                                std::wstring wsettings(header_value);
                                ::CoTaskMemFree(header_value);
                                g_data.host_data.settings = tools::str::wide_to_utf8(wsettings);
                                LogS("Settings loaded: " << g_data.host_data.settings);
                            }
                        }

                        ::Microsoft::WRL::ComPtr<::IStream> content_stream;
                        hres = req->get_Content(&content_stream);
                        if (SUCCEEDED(hres) && content_stream)
                        {
                            g_data.dest_data = read_stream(content_stream.Get());
                            if (!g_data.dest_data.empty())
                            {
                                LogS("Content received: " << g_data.dest_data.size() << " bytes");
                                if (save_dest_image())
                                {
                                    hres = send_response(false, nullptr, env, args, 200, L"OK", "save_mask");
                                    g_data.image_ready.store(true, std::memory_order_relaxed);
                                    return hres;
                                }

                                hres = send_response(false, nullptr, env, args, 500, L"Fail", "save_mask");
                                return hres;
                            }
                        }
                        else
                        {
                            LogS("ICoreWebView2WebResourceRequest::get_Content(save_mask) failed. HR: " << LogHRES(hres));
                        }

                        hres = send_response(false, nullptr, env, args, 500, L"Fail", "save_mask");
                        return hres;
                    }

                    if (uri.starts_with(L"http://app.local/detect") && L"POST" == method)
                    {
                        ::Microsoft::WRL::ComPtr<::IStream> content_stream;
                        hres = req->get_Content(&content_stream);
                        if (SUCCEEDED(hres) && content_stream)
                        {
                            const auto json = read_stream(content_stream.Get());
                            if (!json.empty())
                            {
                                ::Microsoft::WRL::ComPtr<::ICoreWebView2Deferral> deferral;
                                if (SUCCEEDED(args->GetDeferral(&deferral)))
                                {
                                    auto args_copy = ::Microsoft::WRL::ComPtr<::ICoreWebView2WebResourceRequestedEventArgs>(args);
                                    auto env_copy = ::Microsoft::WRL::ComPtr<::ICoreWebView2Environment>(env);

                                    std::thread([json, args_copy, env_copy, deferral, send_response]()
                                    {
                                        auto mask_data = sam_detect(json);
                                        auto* ui_task = new std::function<void()>([args_copy, env_copy, deferral, send_response, mask_data = std::move(mask_data)]() mutable
                                        {
                                            if (!mask_data.empty())
                                            {
                                                g_data.mask_data = std::move(mask_data);
                                                LogS("Mask: " << g_data.mask_data.size() << " bytes");
                                                ::Microsoft::WRL::ComPtr<::IStream> stream;
                                                stream.Attach(::SHCreateMemStream(cast::ptrs<byte>(g_data.mask_data.data()), cast::to<UINT>(g_data.mask_data.size())));
                                                if (stream)
                                                    send_response(true, stream.Get(), env_copy.Get(), args_copy.Get(), 200, L"OK", "detect");
                                                else
                                                    send_response(false, nullptr, env_copy.Get(), args_copy.Get(), 500, L"Fail", "!detect_stream");
                                            }
                                            else
                                            {
                                                send_response(false, nullptr, env_copy.Get(), args_copy.Get(), 500, L"Fail", "!detect");
                                            }

                                            deferral->Complete();
                                        });

                                        ::PostMessage(g_data.hwnd, USER_MSG_INVOKE, 0, cast::from_ptr<LPARAM>(ui_task));
                                    }).detach();

                                    return S_OK;
                                }
                            }
                        }

                        return send_response(false, nullptr, env, args, 500, L"Fail", "detect_fallback");
                    }

                    return hres;
                }).Get(), nullptr);

                if (FAILED(hres))
                {
                    LogS("ICoreWebView2::add_WebResourceRequested failed. HR: " << LogHRES(hres));
                    return hres;
                }

                LogS("Navigating to html: " << g_data.html_data.size() << " bytes");
                hres = g_data.webview->NavigateToString(g_data.html_data.c_str());
                g_data.html_data.clear(); g_data.html_data.shrink_to_fit();
                if (FAILED(hres))
                {
                    LogS("ICoreWebView2::NavigateToString failed. HR: " << LogHRES(hres));
                    return hres;
                }
                return hres;

            }).Get());
            if (FAILED(hres))
            {
                LogS("ICoreWebView2Environment::CreateCoreWebView2Controller failed. HR: " << LogHRES(hres));
                return hres;
            }
            return hres;

        }).Get());
        if (FAILED(hres))
        {
            LogS("CreateCoreWebView2EnvironmentWithOptions failed. HR: " << LogHRES(hres));
            return 1;
        }
    }

    ::MSG msg;
    while (::GetMessage(&msg, nullptr, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    tools::path::remove(data_folder);

    return 0;
}

