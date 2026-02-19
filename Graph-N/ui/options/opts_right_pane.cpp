#include "ui/options/opts_right_pane.h"
#include "ui/graph-n_flow.h"
#include "core/managers/settings_manager.h"
#include "core/managers/comfyui_manager.h"

namespace ui
{

enum
{
    BuffHost,
    BuffMainpy,
    BuffPython,
    BuffWorkDir,
    BuffOutput,
    BuffPythonArgs,
    BuffComfyUIArgs,
    BuffDetectRes,

    _s_count
};

enum
{
    BuffFpsLimit,
    BuffScale,
    BuffConnOptChanged,
    BuffPort,
    BuffConnTm,
    BuffReadTm,
    BuffFpsW,
    BuffFpsC,
    BuffLRamW,
    BuffLRamC,
    BuffSRamW,
    BuffSRamC,
    BuffVRamW,
    BuffVRamC,
    BuffDiskW,
    BuffDiskC,
    BuffPyTorchW,
    BuffPyTorchC,
    BuffPingW,
    BuffPingC,   
    BuffLinkThickness,
    BuffShowDetectRes,
    BuffDetectResOK,

    _i_count
};

OptRightPane::~OptRightPane()
{
    LogF;
}

void OptRightPane::init(OptsData& data)
{
    m_data = &data;

    m_data->buffer_s.resize(_s_count);
    m_data->buffer_i.resize(_i_count);

    m_data->buffer_s[BuffHost] = g::pOptsManager->comfyui_host_load();
    m_data->buffer_s[BuffMainpy] = g::pOptsManager->comfyui_main_py_load().u8string();
    m_data->buffer_s[BuffPython] = g::pOptsManager->comfyui_python_load().u8string();
    m_data->buffer_s[BuffPythonArgs] = g::pOptsManager->comfyui_python_args_load();
    m_data->buffer_s[BuffComfyUIArgs] = g::pOptsManager->comfyui_comfyui_args_load();
    m_data->buffer_s[BuffWorkDir] = g::pOptsManager->comfyui_workdir_load().u8string();
    m_data->buffer_s[BuffOutput] = g::pOptsManager->comfyui_output_load().u8string();

    m_data->buffer_i[BuffFpsLimit] = g::pOptsManager->app_fps_limit_val_load();
    m_data->buffer_i[BuffScale] = g::pOptsManager->app_scale_load();
    m_data->buffer_i[BuffConnOptChanged] = 0;
    m_data->buffer_i[BuffPort] = g::pOptsManager->comfyui_port_load();
    m_data->buffer_i[BuffConnTm] = g::pOptsManager->comfyui_connect_timeout_load();
    m_data->buffer_i[BuffReadTm] = g::pOptsManager->comfyui_read_timeout_load();
    m_data->buffer_i[BuffFpsW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalFPS);
    m_data->buffer_i[BuffFpsC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalFPS);
    m_data->buffer_i[BuffLRamW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalRAM);
    m_data->buffer_i[BuffLRamC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalRAM);
    m_data->buffer_i[BuffSRamW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerRAM);
    m_data->buffer_i[BuffSRamC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerRAM);
    m_data->buffer_i[BuffVRamW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerVRAM);
    m_data->buffer_i[BuffVRamC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerVRAM);
    m_data->buffer_i[BuffDiskW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalDisk);
    m_data->buffer_i[BuffDiskC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalDisk);
    m_data->buffer_i[BuffPyTorchW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerPyTorch);
    m_data->buffer_i[BuffPyTorchC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerPyTorch);
    m_data->buffer_i[BuffPingW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerPing);
    m_data->buffer_i[BuffPingC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerPing);
    m_data->buffer_i[BuffLinkThickness] = g::pOptsManager->wf_link_thickness_load();

    m_data->input.hovered_items.resize(20);
    m_data->input.numbers = g::pOptsManager->wf_widget_input_numbers_load();
    m_data->input.booleans = g::pOptsManager->wf_widget_input_bools_load();

    m_clr_subsep = ImGuiEx::decrease_brightness(g::pOptsManager->color_load(AppColor::OptSeparator));
}

void OptRightPane::init_sizes()
{
    m_data->input_offset_x = 180.0f;
    m_data->input_small_offset_x = 300.0f;
    m_data->help_mark_w = 29.0f;
    m_data->browse_btn_w = 28.0f;

    const float right_pane_w = ImGui::GetContentRegionAvail().x + ImGui::GetStyle().ItemSpacing.x * 2;

    m_data->input_w = right_pane_w - m_data->input_offset_x - ImGui::GetStyle().ItemSpacing.x;
    m_data->input_small_w = right_pane_w - m_data->input_small_offset_x - ImGui::GetStyle().ItemSpacing.x;
    m_data->input_with_browse_w = m_data->input_w - m_data->browse_btn_w - ImGui::GetStyle().ItemSpacing.x;
    m_data->full_row_input_w = right_pane_w - ImGui::GetStyle().ItemSpacing.x * 2;
    m_data->full_row_input_with_browse_w = m_data->full_row_input_w - m_data->browse_btn_w - ImGui::GetStyle().ItemSpacing.x;

}

void OptRightPane::show(OptsData& /*data*/)
{
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(230, 230, 230, 255));

    const char* help_text = nullptr;
    switch (cast::to_enum<OptPane>(m_data->left_selected_item))
    {
    case OptPane::Application:
        show_general(help_text);
        show_rendering(help_text);
        break;
    case OptPane::Connection:
        show_connection(help_text);
        break;
    case OptPane::LocalServer:
        show_local_server(help_text);
        break;
    case OptPane::Interface:
        show_widget_input(help_text);
        show_canvas(help_text);
        show_activity_bar(help_text);
        break;
    case OptPane::Diagnostics:
        show_thresholds(help_text);
        break;
    case OptPane::KeyBindings:
        show_keys(help_text);
        break;
    }

    ImGui::PopStyleColor();

    show_help_text(help_text);
}

// autoupdate
void OptRightPane::show_general(const char*& /*help_text*/)
{
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Common");
    ImGui::Spacing();

    // auto update
    bool b_val = g::pOptsManager->app_autoupdate_load();
    if (ImGui::Checkbox("Check for " APP_CAPTION " updates on startup", &b_val))
        g::pOptsManager->app_autoupdate_save(b_val);
}

// vsync
void OptRightPane::show_rendering(const char*& help_text)
{
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Rendering");
    ImGui::Spacing();

    // vsync
    bool b_val = g::pOptsManager->app_vsync_load();
    if (ImGui::Checkbox("Enable VSync", &b_val))
        g::pOptsManager->app_vsync_save(b_val);
    show_help_mark(help_text,
        "Synchronizes the frame rate with your monitor's refresh rate.\n"
        "Keep this enabled for a smooth, tear-free image.\n"
        "Disable it only if you have a weak GPU and experience stuttering, as it allows the interface to run at its own pace.\n\n"
        "Note: Disabling vsync will show artificially high FPS and increase GPU load; refer to Latency for real performance.");

    // frame rate limit
    if (g::pOptsManager->app_vsync_load())
        ImGui::BeginDisabled();

    b_val = g::pOptsManager->app_fps_limit_load();
    if (ImGui::Checkbox("Frame Rate Limit:", &b_val))
        g::pOptsManager->app_fps_limit_save(b_val);
    show_help_mark(help_text,
        "Caps the maximum frame rate to save power and reduce GPU temperature.");

    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    ImGuiEx::SliderIntSmart("##FpsLimit", &m_data->buffer_i[BuffFpsLimit], 30, 240, "%u FPS");
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->app_fps_limit_val_save(m_data->buffer_i[BuffFpsLimit]);

    if (g::pOptsManager->app_vsync_load())
        ImGui::EndDisabled();

    // scale
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("UI scale");
    show_help_mark(help_text,
        "Adjusts the global scale of the user interface. Useful for High-DPI (4K) displays to make text and controls legible.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    ImGuiEx::SliderIntSmart("##scale", &m_data->buffer_i[BuffScale], 50, 200, "%u %%", 10);
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->app_scale_save(m_data->buffer_i[BuffScale]);
}

// comfyui connection
void OptRightPane::show_connection(const char*& help_text)
{
    ///////////////       ComfyUI address
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), APP_COMFYUI " Address");
    ImGui::Spacing();


    ///////////////       host
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Host:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    m_data->buffer_i[BuffConnOptChanged] =
        ImGuiEx::InputTextWithHint("##ComfyUIHost", "127.0.0.1", &m_data->buffer_s[BuffHost], ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsNoBlank) ||
        m_data->buffer_i[BuffConnOptChanged];

    ///////////////       port
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Port:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    m_data->buffer_i[BuffConnOptChanged] =
        ImGuiEx::SliderIntSmart("##ComfyUIPort", &m_data->buffer_i[BuffPort], 1, 65535, "%u") ||
        m_data->buffer_i[BuffConnOptChanged];


    /////////////////       timeouts
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), APP_COMFYUI " Server Timeouts");
    ImGui::Spacing();


    ///////////////////       connection timeout
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Connect timeout:");
    show_help_mark(help_text,
        "Max time (seconds) to wait for the server to accept a connection.\n\n"
        "Local servers usually connect instantly (1-2s).");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    m_data->buffer_i[BuffConnOptChanged] =
        ImGuiEx::SliderIntSmart("##ComfyUIConnTime", &m_data->buffer_i[BuffConnTm], 0, 30, "%llu sec") ||
        m_data->buffer_i[BuffConnOptChanged];


    ///////////////////       read timeout
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Read timeout:");
    show_help_mark(help_text,
        "Max time (seconds) to wait for a response (e.g., JSON workflow).\n\n"
        "Increase this if loading large workflows over a slow network.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    m_data->buffer_i[BuffConnOptChanged] =
        ImGuiEx::SliderIntSmart("##ComfyUIReadTime", &m_data->buffer_i[BuffReadTm], 0, 30, "%llu sec") ||
        m_data->buffer_i[BuffConnOptChanged];


    /////////////// apply
    if (show_full_row_button("Save/Apply connection settings", m_data->buffer_i[BuffConnOptChanged] && !m_data->buffer_s[BuffHost].empty(), true))
    {
        g::pOptsManager->comfyui_host_save(m_data->buffer_s[BuffHost]);
        g::pOptsManager->comfyui_port_save(m_data->buffer_i[BuffPort]);
        g::pOptsManager->comfyui_connect_timeout_save(m_data->buffer_i[BuffConnTm]);
        g::pOptsManager->comfyui_read_timeout_save(m_data->buffer_i[BuffReadTm]);
        m_data->buffer_i[BuffConnOptChanged] = 0;
    }
    ImGui::SameLine();
    if (show_full_row_button(g::pFlow->get_online() ? "Reconnect to server" : "Connect to server",
        !g::pOptsManager->comfyui_host_load().empty(), true))
    {
        //TODO
        //g::pFlow->reconnect();
    }
}

// local comfyui paths
void OptRightPane::show_local_server(const char*& help_text)
{
    /////////////////       server paths
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), APP_COMFYUI " Server Paths");
    ImGui::Spacing();

    // Detect
    ImGui::TextWrapped("These settings allow " APP_CAPTION " to launch and manage the " APP_COMFYUI " server automatically.\n"
        "You can configure the paths manually below, or launch " APP_COMFYUI " externally and click 'Import Config from Active Process' to auto-detect the settings.");
    const auto y = ImGui::GetCursorPosY();
    if (show_full_row_button("Detect and import config from active process", true, false))
    {
        import_from_comfyui();
    }
    if (m_data->buffer_i[BuffShowDetectRes])
    {
        ImGui::PushStyleColor(ImGuiCol_Text, m_data->buffer_i[BuffDetectResOK] ? IM_COL32(0, 150, 0, 255) : IM_COL32(200, 0, 0, 255));
        ImGui::TextWrapped(m_data->buffer_s[BuffDetectRes].c_str());
        ImGui::PopStyleColor();
    }
    ImGui::Spacing();

    // work dir
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Working directory:");
    show_help_mark(help_text,
        "The root directory for execution context.\n"
        "Usually the folder containing the 'ComfyUI' directory or the 'python_embeded' folder.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_with_browse_w);
    ImGuiEx::InputTextWithHint("##workdir", "C:\\" APP_COMFYUI "_dir", &m_data->buffer_s[BuffWorkDir], ImGuiInputTextFlags_None);
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->comfyui_workdir_save(m_data->buffer_s[BuffWorkDir]);
    ImGui::SameLine();
    if (ImGui::Button("...##browseworkdir", ImVec2(m_data->browse_btn_w, 0)))
        browse_for_workdir();

    // python
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Python path:");
    show_help_mark(help_text,
        "Path to the Python interpreter executable.\n"
        "For portable installations, point to the embedded 'python.exe', not the system one.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_with_browse_w);
    ImGuiEx::InputTextWithHint("##python", "C:\\" APP_COMFYUI "_dir\\python_embedded\\python.exe", &m_data->buffer_s[BuffPython], ImGuiInputTextFlags_None);
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->comfyui_python_save(m_data->buffer_s[BuffPython]);
    ImGui::SameLine();
    if (ImGui::Button("...##browsepython", ImVec2(m_data->browse_btn_w, 0)))
        browse_for_python();

    // Python args
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Python launch args:");
    show_help_mark(help_text,
        "Additional command-line arguments passed to the python.exe (e.g., -s).");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    ImGuiEx::InputTextWithHint("##pythonargs", "-s",
        &m_data->buffer_s[BuffPythonArgs], ImGuiInputTextFlags_None);
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->comfyui_python_args_save(m_data->buffer_s[BuffPythonArgs]);

    // main.py
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Server path:");
    show_help_mark(help_text,
        "Full path to the 'main.py' script located in the " APP_COMFYUI " root folder");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_with_browse_w);
    ImGuiEx::InputTextWithHint("##mainpy", "C:\\" APP_COMFYUI "_dir\\" APP_COMFYUI "\\main.py", &m_data->buffer_s[BuffMainpy], ImGuiInputTextFlags_None);
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->comfyui_main_py_save(m_data->buffer_s[BuffMainpy]);
    ImGui::SameLine();
    if (ImGui::Button("...##browsemainpy", ImVec2(m_data->browse_btn_w, 0)))
        browse_for_mainpy();

    // main.py args
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("ComfyUI args:");
    show_help_mark(help_text,
        "Additional command-line arguments passed to the server (e.g., --lowvram, --cpu, --preview-method auto).");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    ImGuiEx::InputTextWithHint("##confyuiargs", "--windows-standalone-build --disable-auto-launch",
        &m_data->buffer_s[BuffComfyUIArgs], ImGuiInputTextFlags_None);
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->comfyui_comfyui_args_save(m_data->buffer_s[BuffComfyUIArgs]);

    // start
    if (show_full_row_button("Start " APP_COMFYUI,
        !m_data->buffer_s[BuffMainpy].empty() && !m_data->buffer_s[BuffPython].empty() && !m_data->buffer_s[BuffWorkDir].empty(), true))
    {
        g::pComfyUIManager->run(true);
    }
    ImGui::SameLine();
    std::string caption = "Stop " APP_COMFYUI;
    if (g::pComfyUIManager->get_process_started())
        caption = fmt::format("Stop " APP_COMFYUI " [PID: {}]", g::pComfyUIManager->get_started_proc_id());
    if (show_full_row_button(caption.c_str(), g::pComfyUIManager->get_process_started(), true))
    {
        g::pComfyUIManager->kill(true);
    }


    ImGuiEx::Spacing(3);

    /////////////       auto start
    bool b_val = g::pOptsManager->comfyui_start_server_load();
    if (ImGui::Checkbox("Auto-start server", &b_val))
    {
        g::pOptsManager->comfyui_start_server_save(b_val);
        if (!b_val)
        {
            g::pOptsManager->comfyui_kill_server_save(false);
            g::pComfyUIManager->setup_auto_kill(false);
            g::pOptsManager->comfyui_hide_console_save(false);
            g::pOptsManager->comfyui_capture_output_save(false);
            g::pOptsManager->comfyui_autohide_output_save(false);
            g::pOptsManager->comfyui_parse_address_save(false);
        }
    }
    show_help_mark(help_text,
        "Automatically launch the " APP_COMFYUI " server when " APP_CAPTION " starts.");

    /////////////       auto kill
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->full_row_input_w / 2);
    b_val = g::pOptsManager->comfyui_kill_server_load();
    if (ImGui::Checkbox("Auto-stop on exit", &b_val))
    {
        g::pOptsManager->comfyui_kill_server_save(b_val);
        g::pComfyUIManager->setup_auto_kill(b_val);
        if (b_val)
            g::pOptsManager->comfyui_start_server_save(true);
        else
            g::pOptsManager->comfyui_hide_console_save(false);
    }
    show_help_mark(help_text,
        "Terminate the " APP_COMFYUI " process when closing " APP_CAPTION ".\n\n"
        "Prevents orphan python processes running in the background.");

    /////////////       hide console
    b_val = g::pOptsManager->comfyui_hide_console_load();
    if (ImGui::Checkbox("Hide console", &b_val))
    {
        g::pOptsManager->comfyui_hide_console_save(b_val);
        if (b_val)
        {
            g::pOptsManager->comfyui_start_server_save(true);
            g::pOptsManager->comfyui_kill_server_save(true);
            g::pComfyUIManager->setup_auto_kill(true);
            g::pOptsManager->comfyui_capture_output_save(true);
        }
    }
    show_help_mark(help_text,
        "Hides the system command prompt window.\n"
        "Requires 'Capture output' to be enabled to view server logs within " APP_CAPTION ".");

    /////////////       capture output
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->full_row_input_w / 2);
    b_val = g::pOptsManager->comfyui_capture_output_load();
    if (ImGui::Checkbox("Capture output", &b_val))
    {
        g::pOptsManager->comfyui_capture_output_save(b_val);
        if (b_val)
            g::pOptsManager->comfyui_start_server_save(true);
        else
        {
            g::pOptsManager->comfyui_hide_console_save(false);
            g::pOptsManager->comfyui_parse_address_save(false);
        }
    }
    show_help_mark(help_text,
        "Redirect server stdout and stderr to the " APP_CAPTION " Logs panel.\n\n"
        "Allows monitoring errors and progress without the system console.");

    /////////////       auto hide log panel
    b_val = g::pOptsManager->comfyui_autohide_output_load();
    if (ImGui::Checkbox("Autohide Logs panel", &b_val))
    {
        g::pOptsManager->comfyui_autohide_output_save(b_val);
        if (b_val)
        {
            g::pOptsManager->comfyui_start_server_save(true);
            g::pOptsManager->comfyui_capture_output_save(true);
        }
    }
    show_help_mark(help_text,
        "Automatically shows the Logs panel when " APP_COMFYUI " is disconnected and hides it after a successful connection.\n"
        "You can always toggle the panel manually from the main menu.");

    /////////////       parse address
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->full_row_input_w / 2);
    b_val = g::pOptsManager->comfyui_parse_address_load();
    if (ImGui::Checkbox("Auto-detect host/port", &b_val))
    {
        g::pOptsManager->comfyui_parse_address_save(b_val);
        if (b_val)
        {
            g::pOptsManager->comfyui_start_server_save(true);
            g::pOptsManager->comfyui_capture_output_save(true);
        }
    }
    show_help_mark(help_text,
        "Attempt to find the listening IP and port by parsing the server startup logs.\n\n"
        "Overrides manual Connection settings if successful.");

    ImGui::Spacing();

    // output folder
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Output directory:");
    show_help_mark(help_text,
        "Optional.\n"
        "Path to the " APP_COMFYUI " output folder. Enables direct disk access for instant image loading, bypassing network requests.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_with_browse_w);
    ImGuiEx::InputTextWithHint("##output", "C:\\" APP_COMFYUI "_install\\" APP_COMFYUI "\\output", &m_data->buffer_s[BuffOutput], ImGuiInputTextFlags_None);
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->comfyui_output_save(m_data->buffer_s[BuffOutput]);
    ImGui::SameLine();
    if (ImGui::Button("...##browseoutput", ImVec2(m_data->browse_btn_w, 0)))
        browse_for_outputdir();
}

// input styles
void OptRightPane::show_widget_input(const char*& help_text)
{
    const auto draw_selected = [](bool selected, bool& hovered, int32_t id, const char* text)
    {
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));
            ImGui::TextUnformatted(ICON_LC_CHECK_CHECK);
            ImGui::SameLine();
            ImGui::TextUnformatted(text);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushID(id);
            if (hovered)    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(text);
            if (hovered)    ImGui::PopStyleColor();
            ImGui::PopID();
            hovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                return true;
        }
        return false;
    };

    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::OptSeparator));
    ImGui::TextUnformatted("Default Input Styles");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    ///////////////  
    int32_t idx = 0;

    ///////////////       numbers
    ImGuiEx::Separator(m_clr_subsep, "Numbers");
    ImGui::Spacing();


    double d_min = 5.0;
    double d_max = 142.0;

    if (draw_selected(WidgetInput::Direct == m_data->input.numbers, m_data->input.hovered_items[idx], idx + 1, "Input field:"))
    {
        m_data->input.numbers = WidgetInput::Direct;
        g::pOptsManager->wf_widget_input_numbers_save(m_data->input.numbers);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::InputDoubleSmart("##WfNumbers0", &m_data->input.d_sample, 0.01, d_min, d_max, " Value: %.4f", "%.4f");


    if (draw_selected(WidgetInput::Slider == m_data->input.numbers, m_data->input.hovered_items[idx], idx + 1, "Slider:"))
    {
        m_data->input.numbers = WidgetInput::Slider;
        g::pOptsManager->wf_widget_input_numbers_save(m_data->input.numbers);
    }
    idx++;
    show_help_mark(help_text, "Ctrl+click to type value.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    ImGuiEx::SliderDoubleSmart("##WfNumbers1", &m_data->input.d_sample, d_min, d_max, "Value: %.4f");


    if (draw_selected(WidgetInput::Drag == m_data->input.numbers, m_data->input.hovered_items[idx], idx + 1, "Scrubber:"))
    {
        m_data->input.numbers = WidgetInput::Drag;
        g::pOptsManager->wf_widget_input_numbers_save(m_data->input.numbers);
    }
    idx++;
    show_help_mark(help_text, "Ctrl+click or double click to type value.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    ImGuiEx::DragDoubleSmart("##WfNumbers2", &m_data->input.d_sample, d_min, d_max, "Value: %.4f");


    ImGui::Spacing();


    ///////////////       booleans
    ImGuiEx::Separator(m_clr_subsep, "Booleans");
    ImGui::Spacing();


    bool b_val = !!m_data->input.b_sample;

    if (draw_selected(WidgetInput::Select == m_data->input.booleans, m_data->input.hovered_items[idx], idx + 1, "Selectable:"))
    {
        m_data->input.booleans = WidgetInput::Select;
        g::pOptsManager->wf_widget_input_bools_save(m_data->input.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    if (ImGuiEx::BooleanToggleLED(b_val ? "Value: on##WfBool1" : "Value: off##WfBool1", &b_val, false, false))
        m_data->input.b_sample = b_val;


    if (draw_selected(WidgetInput::Toggle == m_data->input.booleans, m_data->input.hovered_items[idx], idx + 1, "Toggle button:"))
    {
        m_data->input.booleans = WidgetInput::Toggle;
        g::pOptsManager->wf_widget_input_bools_save(m_data->input.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    if (ImGuiEx::BooleanButtons2("##WfBool2", b_val ? "Value: on" : "Value: off", &b_val))
        m_data->input.b_sample = b_val;

}

// grid, snap, theme
void OptRightPane::show_canvas(const char*& help_text)
{
    // Canvas
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::OptSeparator));
    ImGui::TextUnformatted("Canvas");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // show grid
    bool b_val = g::pOptsManager->wf_show_grid_load();
    if (ImGui::Checkbox("Show grid", &b_val))
        g::pOptsManager->wf_show_grid_save(b_val);
    show_help_mark(help_text, "Toggles the visibility of the background grid in the node editor.");

    // snap nodes
    b_val = g::pOptsManager->wf_snap_nodes_load();
    if (ImGui::Checkbox("Snap to grid", &b_val))
        g::pOptsManager->wf_snap_nodes_save(b_val);
    show_help_mark(help_text, "Aligns nodes to the nearest grid lines when dragging."); // \nHold Alt to temporarily invert this behavior.

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Link Thickness:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::SetNextItemWidth(m_data->input_w);
    ImGuiEx::SliderIntSmart("##Thickness", &m_data->buffer_i[BuffLinkThickness], 1, 5, "%u");
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->wf_link_thickness_save(m_data->buffer_i[BuffLinkThickness]);

    //// theme
    //ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Theme");

    //const auto theme = g::pOptsManager->ui_theme_load();
    //if (ImGui::RadioButton("System default", 0 == theme))
    //    g::pOptsManager->ui_theme_save(0);
    //if (ImGui::RadioButton("Dark", 1 == theme))
    //    g::pOptsManager->ui_theme_save(1);
    //if (ImGui::RadioButton("Light", 2 == theme))
    //    g::pOptsManager->ui_theme_save(2);
}

void OptRightPane::show_activity_bar(const char*& help_text)
{
    // activity bar
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::OptSeparator));
    ImGui::TextUnformatted("Activity Bar");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // mode
    auto style = g::pOptsManager->ui_activity_bar_mode_load();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Style");

    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    if (ImGui::RadioButton("Left only", ActivityBarMode::Left == style))
    {
        g::pOptsManager->ui_activity_bar_mode_save(ActivityBarMode::Left);
        g::pFlow->load_activity_bar_mode();
    }
    show_help_mark(help_text, "Force all icons to the left side.");

    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->full_row_input_w / 2);
    if (ImGui::RadioButton("Right only", ActivityBarMode::Right == style))
    {
        g::pOptsManager->ui_activity_bar_mode_save(ActivityBarMode::Right);
        g::pFlow->load_activity_bar_mode();
    }
    show_help_mark(help_text, "Force all icons to the right side.");

    ImGui::SetCursorPosX(m_data->input_offset_x);
    if (ImGui::RadioButton("Smart", ActivityBarMode::Smart == style))
    {
        g::pOptsManager->ui_activity_bar_mode_save(ActivityBarMode::Smart);
        g::pFlow->load_activity_bar_mode();
    }
    show_help_mark(help_text, "Show bars on sides where panels are docked.");

    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->full_row_input_w / 2);
    if (ImGui::RadioButton("Hidden", ActivityBarMode::Disable == style))
    {
        g::pOptsManager->ui_activity_bar_mode_save(ActivityBarMode::Disable);
        g::pFlow->load_activity_bar_mode();
    }
    show_help_mark(help_text, "Maximize workspace space.");

    // size
    ImGui::Spacing();
    auto big_icons = g::pOptsManager->ui_activity_bar_big_icons_load();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Icons size");

    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    if (ImGui::RadioButton("Big", big_icons))
    {
        g::pOptsManager->ui_activity_bar_big_icons_save(true);
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->full_row_input_w / 2);
    if (ImGui::RadioButton("Small", !big_icons))
    {
        g::pOptsManager->ui_activity_bar_big_icons_save(false);
    }

    // restart
    //ImGui::Spacing();
    //ImGui::AlignTextToFramePadding();
    //ImGui::TextUnformatted("Additional");

    //ImGui::SameLine();
    //ImGui::SetCursorPosX(m_data->input_offset_x);
    //bool b_val = g::pOptsManager->ui_activity_bar_show_restart_load();
    //if (ImGui::Checkbox("Add 'Restart " APP_COMFYUI "' button", &b_val))
    //    g::pOptsManager->ui_activity_bar_show_restart_save(b_val);
}

// stats
void OptRightPane::show_thresholds(const char*& help_text)
{
    const auto bytes_to_gb = [](int64_t bytes) -> double
    {
        return cast::to_double(bytes) / (1024.0 * 1024.0 * 1024.0);
    };

    const auto gb_to_bytes = [](double gb) -> int64_t
    {
        return cast::to_i64(gb * 1024.0 * 1024.0 * 1024.0);
    };

    const auto show_thresholds = [&bytes_to_gb, &gb_to_bytes, data = m_data](
        const char* warn_name, const char* crit_name,
        int64_t& warn, int64_t& crit, bool convert_bytes, bool div_by_100,
        float min, float max, const char* format)
    {
        std::pair<bool, bool> changed = std::make_pair(false, false);
        const float slider_w = data->input_w / 2;

        ImGui::SameLine();
        ImGui::SetCursorPosX(data->input_offset_x);

        ImGui::SetNextItemWidth(slider_w);
        double val = convert_bytes ? bytes_to_gb(warn) : div_by_100 ? cast::to_double(warn) / 100 : cast::to_double(warn);
        if (ImGuiEx::SliderDoubleSmart(warn_name, &val, min, max, format))
            warn = convert_bytes ? gb_to_bytes(val) : div_by_100 ? cast::to_i64(val * 100) : cast::to_i64(val);
        changed.first = ImGui::IsItemDeactivatedAfterEdit();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_w);
        val = convert_bytes ? bytes_to_gb(crit) : div_by_100 ? cast::to_double(crit) / 100 : cast::to_double(crit);
        if (ImGuiEx::SliderDoubleSmart(crit_name, &val, min, max, format))
            crit = convert_bytes ? gb_to_bytes(val) : div_by_100 ? cast::to_i64(val * 100) : cast::to_i64(val);
        changed.second = ImGui::IsItemDeactivatedAfterEdit();

        return changed;
    };

    ImGui::TextWrapped(
        "Set limits for system resources.\n"
        "Values exceeding these limits will be highlighted in red in the Stats panel.");

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator));

    ImGui::SetCursorPosX(m_data->input_offset_x);
    ImGui::TextUnformatted("Warnings");
    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x + ImGui::GetStyle().ItemSpacing.x + m_data->input_w / 2);
    ImGui::TextUnformatted("Criticals");

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Server Threasholds");

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("RAM:");
    show_help_mark(help_text,
        "Thresholds for free/available memory.\n"
        "If available memory drops below this value, the system status will be flagged.\n"
        "Warning: Low memory risks slowdowns.\n"
        "Critical: Out Of Memory (OOM) crashes are imminent.");
    auto changed = show_thresholds("##SRAMWarning", "##SRAMCritical", m_data->buffer_i[BuffSRamW], m_data->buffer_i[BuffSRamC], true, false, 1.0f, 10.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::ServerRAM, m_data->buffer_i[BuffSRamW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::ServerRAM, m_data->buffer_i[BuffSRamC]);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("VRAM:");
    show_help_mark(help_text,
        "Thresholds for free/available memory.\n"
        "If available memory drops below this value, the system status will be flagged.\n"
        "Warning: Low memory risks slowdowns.\n"
        "Critical: Out Of Memory (OOM) crashes are imminent.");
    changed = show_thresholds("##VRAMWarning", "##VRAMCritical", m_data->buffer_i[BuffVRamW], m_data->buffer_i[BuffVRamC], true, false, 0.1f, 5.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::ServerVRAM, m_data->buffer_i[BuffVRamW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::ServerVRAM, m_data->buffer_i[BuffVRamC]);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("PyTorch VRAM:");
    show_help_mark(help_text,
        "Thresholds for free/available memory.\n"
        "If available memory drops below this value, the system status will be flagged.\n"
        "Warning: Low memory risks slowdowns.\n"
        "Critical: Out Of Memory (OOM) crashes are imminent.");
    changed = show_thresholds("##PyTorchWarning", "##PyTorchCritical", m_data->buffer_i[BuffPyTorchW], m_data->buffer_i[BuffPyTorchC], true, false, 0.1f, 5.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::ServerPyTorch, m_data->buffer_i[BuffPyTorchW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::ServerPyTorch, m_data->buffer_i[BuffPyTorchC]);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Ping:");
    show_help_mark(help_text,
        "Network round-trip time + Server processing time.\n"
        "Note: High values on localhost usually indicate the ComfyUI backend is blocked by heavy computations (Python GIL), not network issues.");
    changed = show_thresholds("##pingWarning", "##PingCritical", m_data->buffer_i[BuffPingW], m_data->buffer_i[BuffPingC], false, false, 1.0f, 500.0f, "%.0f ms");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::ServerPing, m_data->buffer_i[BuffPingW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::ServerPing, m_data->buffer_i[BuffPingC]);

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "This PC Threasholds");

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("RAM:");
    show_help_mark(help_text,
        "Thresholds for free/available memory.\n"
        "If available memory drops below this value, the system status will be flagged.\n"
        "Warning: Low memory risks slowdowns.\n"
        "Critical: Out Of Memory (OOM) crashes are imminent.");
    changed = show_thresholds("##LRAMWarning", "##LRAMCritical", m_data->buffer_i[BuffLRamW], m_data->buffer_i[BuffLRamC], true, false, 0.5f, 10.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::LocalRAM, m_data->buffer_i[BuffLRamW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::LocalRAM, m_data->buffer_i[BuffLRamC]);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Disk(s) space:");
    show_help_mark(help_text,
        "Minimum free space threshold for selected drives.\n"
        "Helps prevent generation failures due to full disk (e.g., when saving large batches of images).");
    changed = show_thresholds("##DiskWarning", "##DiskCritical", m_data->buffer_i[BuffDiskW], m_data->buffer_i[BuffDiskC], true, false, 1.0f, 40.0f, "%.0f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::LocalDisk, m_data->buffer_i[BuffDiskW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::LocalDisk, m_data->buffer_i[BuffDiskC]);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Monitored drives:");
    const float disk_w = m_data->input_w / 2;

    ImGui::SameLine();
    ImGui::SetCursorPosX(m_data->input_offset_x);
    if (ImGui::BeginChild("DrivesList", ImVec2(0, 100), true, ImGuiWindowFlags_HorizontalScrollbar)) 
    {
        for (auto& disk : g::pOptsManager->stat_disks_enabled_load())
        {
            if (ImGui::Checkbox(disk.first.c_str(), &disk.second))
                g::pOptsManager->stat_disks_enabled_save();
        }
        ImGui::EndChild();
    }


    //int32_t disks = 1;
    //for (auto& disk : g::pOptsManager->stat_disks_enabled_load())
    //{
    //    if (2 == disks)
    //    {
    //        ImGui::SameLine();
    //        ImGui::SetCursorPosX(m_data->input_offset_x + ImGui::GetStyle().ItemSpacing.x + disk_w);
    //    }

    //    if (ImGui::Checkbox(disk.first.c_str(), &disk.second))
    //    {
    //        g::pOptsManager->stat_disks_enabled_save();
    //    }

    //    if (++disks > 2)
    //    {
    //        disks = 1;
    //        ImGui::SetCursorPosX(m_data->input_offset_x);
    //    }
    //}

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), APP_CAPTION " performance");

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("UI latency:");
    show_help_mark(help_text,
        "Client-side render time (Frame Time).\n"
        "Indicates how long it takes for " APP_CAPTION " to process and draw one frame.\n"
        "High values mean the interface itself is lagging (heavy load on your GPU/CPU).");
    changed = show_thresholds("##LatencyWarning", "##LatencyCritical", m_data->buffer_i[BuffFpsW], m_data->buffer_i[BuffFpsC], false, true, 0.01f, 20.0f, "%.2f ms");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::LocalFPS, m_data->buffer_i[BuffFpsW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::LocalFPS, m_data->buffer_i[BuffFpsC]);

}

// key bindings
void OptRightPane::show_keys(const char*& /*help_text*/)
{}

////////////////////////  helpers

void OptRightPane::show_help_mark(const char*& help_text, const char* hint)
{
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::OptHelpMark));
    const auto pos = ImGui::GetCursorPosX();
    ImGui::TextUnformatted("[?]");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetCursorPosX(pos);
    ImGui::InvisibleButton("[?]hover", ImVec2(m_data->help_mark_w, m_data->help_mark_w));
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        help_text = hint;
}

void OptRightPane::show_help_text(const char* help_text)
{
    if (help_text != nullptr)
    {
        ImGui::Dummy(ImVec2(0.0f, 50.0f));

        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::OptHelpText));
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(help_text);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }
}

bool OptRightPane::show_full_row_button(const char* text, bool enabled, bool half_w)
{
    ImGuiEx::Spacing(2);

    if (!enabled)
        ImGui::BeginDisabled();
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, g::pOptsManager->color_load(AppColor::OptBtnNormal));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g::pOptsManager->color_load(AppColor::OptBtnHover));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g::pOptsManager->color_load(AppColor::OptBtnPressed));
    }
    float w = m_data->full_row_input_w;
    if (half_w)
        w = w / 2 - ImGui::GetStyle().ItemSpacing.x / 2;
    const bool click = ImGui::Button(text, ImVec2(w, 0));
    if (!enabled)
        ImGui::EndDisabled();
    else
        ImGui::PopStyleColor(3);
    return click;
}

void OptRightPane::browse_for_python()
{
    tools::path::FdSelectResultSg callback = [this](bool result, std::filesystem::path&& path)
    {
        if (result)
        {
            m_data->buffer_s[BuffPython] = path.u8string();
            g::pOptsManager->comfyui_python_save(m_data->buffer_s[BuffPython]);
        }
    };
    tools::path::select_file_open({{ L"Executable (*.exe)", L"*.exe" }}, callback);
}

void OptRightPane::browse_for_mainpy()
{
    tools::path::FdSelectResultSg callback = [this](bool result, std::filesystem::path&& path)
    {
        if (result)
        {
            m_data->buffer_s[BuffMainpy] = path.u8string();
            g::pOptsManager->comfyui_main_py_save(m_data->buffer_s[BuffMainpy]);
        }
    };
    tools::path::select_file_open({{ L"main.py", L"main.py" }}, callback);
}

void OptRightPane::browse_for_workdir()
{
    tools::path::FdSelectResultSg callback = [this](bool result, std::filesystem::path&& path)
    {
        if (result)
        {
            m_data->buffer_s[BuffWorkDir] = std::move(path.u8string());
            g::pOptsManager->comfyui_workdir_save(m_data->buffer_s[BuffWorkDir]);
        }
    };
    tools::path::select_dir(callback);
}

void OptRightPane::browse_for_outputdir()
{
    tools::path::FdSelectResultSg callback = [this](bool result, std::filesystem::path&& path)
    {
        if (result)
        {
            m_data->buffer_s[BuffOutput] = std::move(path.u8string());
            g::pOptsManager->comfyui_output_save(m_data->buffer_s[BuffOutput]);
        }
    };
    tools::path::select_dir(callback);
}

void OptRightPane::import_from_comfyui()
{
    m_data->buffer_i[BuffShowDetectRes] = 1;
    m_data->buffer_i[BuffDetectResOK] = 0;
    m_data->buffer_s[BuffDetectRes] = "Please configure the paths below manually.";

    constexpr const char* msg_success = "Configuration imported successfully.";
    constexpr const char* msg_err_no_proc = "No running " APP_COMFYUI " process detected. Please launch " APP_COMFYUI " manually or configure the paths below.";
    constexpr const char* msg_err_no_args = "Detected " APP_COMFYUI ", but could not read process data. Please configure paths manually.";

#ifdef _WIN32
    CComPtr<IWbemLocator> locator;
    auto hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&locator);
    if (FAILED(hres))
    {
        LogWarn("Failed to create IWbemLocator, err: " << LogHEX(hres));
        return;
    }

    CComPtr<IWbemServices> service;
    hres = locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &service);
    if (FAILED(hres))
    {
        LogWarn("Failed to connect ROOT\\CIMV2, err: " << LogHEX(hres));
        return;
    }

    hres = CoSetProxyBlanket(service, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres))
    {
        LogWarn("Failed to configure service, err: " << LogHEX(hres));
        return;
    }

    CComPtr<IEnumWbemClassObject> enumerator;
    hres = service->ExecQuery(_bstr_t(L"WQL"),
        _bstr_t(L"SELECT CommandLine, ExecutablePath, ParentProcessId FROM Win32_Process WHERE Name LIKE '%python%'"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);
    if (FAILED(hres))
    {
        LogWarn("Failed to query python processes, err: " << LogHEX(hres));
        return;
    }

    CComVariant var;
    DWORD parent_proc = 0;
    std::filesystem::path python_path, mainpy_path, work_dir, output_dir;
    std::wstring python_args, cmdexe_args, comfyui_args;

    if (enumerator != nullptr)
    {
        ULONG ret;
        CComPtr<IWbemClassObject> object;
        while (S_OK == enumerator->Next(WBEM_INFINITE, 1, &object, &ret))
        {
            var.Clear();
            hres = object->Get(L"CommandLine", 0, &var, nullptr, nullptr);
            if (FAILED(hres))
            {
                LogWarn("Failed to get python CommandLine, err: " << LogHEX(hres));
                object.Release();
                continue;
            }

            if (VT_BSTR == var.vt && wcsstr(var.bstrVal, L"main.py") != nullptr)
            {
                python_args = var.bstrVal;

                var.Clear();
                hres = object->Get(L"ExecutablePath", 0, &var, nullptr, nullptr);
                if (FAILED(hres) || var.vt != VT_BSTR)
                {
                    LogWarn("Failed to get python ExecutablePath, err: " << LogHEX(hres));
                    continue;
                }
                python_path = var.bstrVal;
                if (!python_path.is_absolute())
                    continue;

                var.Clear();
                hres = object->Get(L"ParentProcessId", 0, &var, nullptr, nullptr);
                if (FAILED(hres) || (var.vt != VT_UI4 && var.vt != VT_I4))
                {
                    LogWarn("Failed to get python ParentProcessId, err: " << LogHEX(hres));
                    continue;
                }
                parent_proc = var.uintVal;
                break;
            }
            object.Release();
        }
    }

    if (0 == parent_proc)
    {
        m_data->buffer_s[BuffDetectRes] = msg_err_no_proc;
        return;
    }

    bool comfyexe_found = false, cmdexe_found = false;

    while (parent_proc != 0)
    {
        enumerator.Release();
        auto req = fmt::format("SELECT Name, CommandLine, ExecutablePath, ParentProcessId FROM Win32_Process WHERE ProcessId = {}", parent_proc);
        hres = service->ExecQuery(_bstr_t(L"WQL"), _bstr_t(req.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);
        if (FAILED(hres) || nullptr == enumerator)
        {
            LogWarn("Failed to query parent processes, err: " << LogHEX(hres));
            break;
        }

        CComPtr<IWbemClassObject> object;
        ULONG ret;
        if (S_OK == enumerator->Next(WBEM_INFINITE, 1, &object, &ret))
        {
            var.Clear();
            hres = object->Get(L"ParentProcessId", 0, &var, nullptr, nullptr);
            if (FAILED(hres) || (var.vt != VT_UI4 && var.vt != VT_I4))
            {
                LogWarn("Failed to get python ParentProcessId, err: " << LogHEX(hres));
                break;
            }
            parent_proc = var.uintVal;

            var.Clear();
            hres = object->Get(L"Name", 0, &var, nullptr, nullptr);
            if (FAILED(hres) || var.vt != VT_BSTR)
            {
                LogWarn("Failed to get process Name, err: " << LogHEX(hres));
                break;
            }
            if (_wcsicmp(var.bstrVal, L"comfyui.exe") == 0) // ComfyUI Electron installation
            {
                comfyexe_found = true;
                break;
            }
            if (_wcsicmp(var.bstrVal, L"cmd.exe") == 0) // probably portable ComfyUI
            {
                var.Clear();
                hres = object->Get(L"CommandLine", 0, &var, nullptr, nullptr);
                if (FAILED(hres) || var.vt != VT_BSTR)
                {
                    LogWarn("Failed to get cmd.exe CommandLine, err: " << LogHEX(hres));
                    break;
                }
                if (wcsstr(var.bstrVal, L".bat") != nullptr) // parent cmd with .bat
                {
                    cmdexe_found = true;
                    cmdexe_args = var.bstrVal;
                    break;
                }
            }
            else if (wcsstr(var.bstrVal, L"python") != nullptr) // parent python, check args
            {
                var.Clear();
                hres = object->Get(L"CommandLine", 0, &var, nullptr, nullptr);
                if (FAILED(hres) || var.vt != VT_BSTR)
                {
                    LogWarn("Failed to get python CommandLine, err: " << LogHEX(hres));
                    break;
                }
                if (wcsstr(var.bstrVal, L"main.py") != nullptr) // parent python. replace with previously found 
                {
                    python_args = var.bstrVal;

                    var.Clear();
                    hres = object->Get(L"ExecutablePath", 0, &var, nullptr, nullptr);
                    if (FAILED(hres) || var.vt != VT_BSTR)
                    {
                        LogWarn("Failed to get python ExecutablePath, err: " << LogHEX(hres));
                        break;
                    }
                    python_path = var.bstrVal;
                    break;
                }
            }
            object.Release();
        }
        else
            break;
    }

    m_data->buffer_s[BuffDetectRes] = msg_err_no_args;
    if (python_args.empty() || !tools::path::exists(python_path) || !python_path.is_absolute())
        return;

    if (!comfyexe_found && cmdexe_found) // portable build ?
    {
        int32_t num = 0;
        auto args = ::CommandLineToArgvW(cmdexe_args.c_str(), &num);
        if (!args)
            return;

        std::filesystem::path bat_path;
        for (int32_t i = 0; i < num; i++)
        {
            if (wcsstr(args[i], L".bat") != nullptr)
            {
                bat_path = args[i];
                break;
            }
        }
        LocalFree(args);

        if (bat_path.empty())
            return;

        work_dir = bat_path.remove_filename();
        if (!tools::path::exists(work_dir) || !work_dir.is_absolute())
        {
            work_dir.clear();
            Log(work_dir << " not found");
        }
        else
        {
            auto mainpy_tmp = work_dir / "ComfyUI" / "main.py";
            if (tools::path::exists(mainpy_tmp))
            {
                mainpy_path = mainpy_tmp;
                mainpy_tmp = work_dir / "ComfyUI" / "output";
                if (tools::path::exists(mainpy_tmp))
                    output_dir = std::move(mainpy_tmp);
            }
        }
    }

    if (!comfyexe_found && tools::path::exists(python_path) && python_path.is_absolute() &&
        (!tools::path::exists(mainpy_path) || !tools::path::exists(work_dir)))
    {
        // ok, search around python if it is embedded
        auto tmp = python_path.parent_path();
        do
        {
            if (tmp.empty() || !tmp.has_relative_path())
                break;
            auto mainpy_tmp = tmp / "ComfyUI" / "main.py";
            if (tools::path::exists(mainpy_tmp))
            {
                mainpy_path = mainpy_tmp;
                work_dir = tmp;

                mainpy_tmp = tmp / "ComfyUI" / "output";
                if (tools::path::exists(mainpy_tmp))
                    output_dir = std::move(mainpy_tmp);
                break;
            }
            else
                tmp = tmp.parent_path();
        }
        while (true);

        if (!tools::path::exists(mainpy_path) || !tools::path::exists(work_dir))
            return;
    }

    int32_t num = 0;
    auto args = ::CommandLineToArgvW(python_args.c_str(), &num);
    if (!args)
        return;

    std::wstring python_args_tmp = L"-s ";
    bool to_comfy_args = false;
    comfyui_args.clear();

    for (int32_t i = 0; i < num; i++)
    {
        if (_wcsicmp(args[i], L"-u") == 0 || _wcsicmp(args[i], L"-s") == 0)  // we will add these flags manually
            continue;
        if (wcsstr(args[i], L".exe") != nullptr && !to_comfy_args) // python.exe
            continue;
        if (wcsstr(args[i], L"main.py") != nullptr)
        {
            if (tools::path::exists(args[i]) && std::filesystem::path(args[i]).is_absolute()) // installer might pass full path
            {
                mainpy_path = args[i];
            }
            to_comfy_args = true;
            continue;
        }
        else if (to_comfy_args)
        {
            comfyui_args += tools::path::build_commandline_arg(args[i]) + L" ";

            // installer might pass full paths
            if (_wcsicmp(args[i], L"--output-directory") == 0 && i + 1 < num)
            {
                std::filesystem::path tmp(args[i + 1]);
                if (tools::path::exists(tmp) && tmp.is_absolute())
                    output_dir = std::move(tmp);
            }
            else if (_wcsicmp(args[i], L"--base-directory") == 0 && i + 1 < num)
            {
                std::filesystem::path tmp(args[i + 1]);
                if (tools::path::exists(tmp) && tmp.is_absolute())
                    work_dir = std::move(tmp);
            }
        }
        else
        {
            python_args_tmp += tools::path::build_commandline_arg(args[i]) + L" ";
        }
    }

    LocalFree(args);

    if (tools::path::exists(python_path) && python_path.is_absolute() &&
        tools::path::exists(mainpy_path) && mainpy_path.is_absolute() &&
        tools::path::exists(work_dir) && work_dir.is_absolute() &&
        !python_args_tmp.empty() && !comfyui_args.empty())
    {
        m_data->buffer_s[BuffPython] = python_path.u8string();
        g::pOptsManager->comfyui_python_save(m_data->buffer_s[BuffPython]);
        m_data->buffer_s[BuffMainpy] = mainpy_path.u8string();
        g::pOptsManager->comfyui_main_py_save(m_data->buffer_s[BuffMainpy]);
        m_data->buffer_s[BuffWorkDir] = work_dir.u8string();
        g::pOptsManager->comfyui_workdir_save(m_data->buffer_s[BuffWorkDir]);
        if (tools::path::exists(output_dir))
        {
            m_data->buffer_s[BuffOutput] = output_dir.u8string();
            g::pOptsManager->comfyui_output_save(m_data->buffer_s[BuffOutput]);
        }
        m_data->buffer_s[BuffPythonArgs] = _bstr_t(python_args_tmp.c_str());
        g::pOptsManager->comfyui_python_args_save(m_data->buffer_s[BuffPythonArgs]);
        m_data->buffer_s[BuffComfyUIArgs] = _bstr_t(comfyui_args.c_str());
        g::pOptsManager->comfyui_comfyui_args_save(m_data->buffer_s[BuffComfyUIArgs]);

        m_data->buffer_i[BuffDetectResOK] = 1;
        m_data->buffer_s[BuffDetectRes] = msg_success;
    }

#else
    m_data->buffer_s[BuffDetectRes] = "Please configure the paths below manually.";
#endif
}

}
