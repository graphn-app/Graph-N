
#include "ui/options/opt_comfyui_dlg.h"
#include "core/managers/settings_manager.h"
#include "ui/helpers/ImFileDialog.h"
#include "ui/graph-n_flow.h"

namespace ui
{

enum
{
    BuffHost,
    BuffCmd,
    BuffPath,
    BuffPython,

    _buff_s_count,
};

enum
{
    BuffPort,
    BuffConnTM,
    BuffReadTM,

    _buff_i_count,
};

OptComfyDlg::~OptComfyDlg()
{
    LogF;
}

void OptComfyDlg::init(OptsData& data)
{
    LogF;

    data.comfyui.buffer_s->resize(_buff_s_count);
    (*data.comfyui.buffer_s)[BuffHost] = g::pOptsManager->comfyui_host_load();
    (*data.comfyui.buffer_s)[BuffPath] = g::pOptsManager->comfyui_main_py_load().u8string();
    (*data.comfyui.buffer_s)[BuffCmd] = g::pOptsManager->comfyui_command_load();
    (*data.comfyui.buffer_s)[BuffPython] = g::pOptsManager->comfyui_python_load();

    data.comfyui.buffer_i->resize(_buff_i_count);
    (*data.comfyui.buffer_i)[BuffPort] = g::pOptsManager->comfyui_port_load();
    (*data.comfyui.buffer_i)[BuffConnTM] = g::pOptsManager->comfyui_connect_timeout_load();
    (*data.comfyui.buffer_i)[BuffReadTM] = g::pOptsManager->comfyui_read_timeout_load();
}

void OptComfyDlg::show(OptsData& data)
{
    constexpr const float padding = 6.0f;
    float avail_x, item_w, offset;
    const char* help_text = nullptr;
    begin_group(data, padding, avail_x, item_w, offset);
    show_connection_opts(data, help_text, padding, avail_x, item_w, offset);
    end_group(padding);
    begin_group(data, padding, avail_x, item_w, offset);
    show_interaction_opts(data, help_text, padding, avail_x, item_w, offset);
    end_group(padding);

    ImGui::Dummy(ImVec2(0.0f, 50.0f));
    show_help_text(help_text);
}

void OptComfyDlg::begin_group(OptsData& data, float padding, float& avail_x, float& item_w, float& offset)
{
    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(padding, padding));

    ImGui::BeginGroup();

    avail_x = ImGui::GetContentRegionAvail().x;
    item_w = avail_x - data.input_offset_x - data.help_mark_w;
    offset = data.input_offset_x + data.help_mark_w;
}

void OptComfyDlg::end_group(float padding)
{
    ImGui::EndGroup();

    ImVec2 p_min = ImGui::GetItemRectMin();
    ImVec2 p_max = ImGui::GetItemRectMax();

    p_min.x -= padding;
    p_min.y -= padding;
    p_max.x += padding; 
    p_max.y += padding;

    ImGui::GetWindowDrawList()->AddRect(p_min, p_max, IM_COL32(100, 100, 100, 255), 4.0f);

    ImGui::Dummy(ImVec2(0, padding));
}

void OptComfyDlg::show_connection_opts(OptsData& data, const char*& help_text, float padding, float avail_x, float item_w, float offset)
{
    auto& buffer_s = *data.comfyui.buffer_s;
    auto& buffer_i = *data.comfyui.buffer_i;


    ///////////////       ComfyUI address
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), avail_x - padding, APP_COMFYUI " Address");
    ImGui::Spacing();


    ///////////////       host
    ImGui::TextUnformatted("Host:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(offset);
    ImGui::SetNextItemWidth(item_w);
    m_data_changed = ImGuiEx::InputText("##ComfyUIHost", &buffer_s[BuffHost], ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsNoBlank) || m_data_changed;


    ///////////////       port
    ImGui::TextUnformatted("Port:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(offset);
    ImGui::SetNextItemWidth(item_w);
    m_data_changed = ImGuiEx::SliderIntSmart("##ComfyUIPort", &buffer_i[BuffPort], 1, 65535, "%u") || m_data_changed;


    /////////////////       timeouts
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), avail_x - padding, APP_COMFYUI " Server Timeouts");
    ImGui::Spacing();


    ///////////////////       connection timeout
    ImGui::TextUnformatted("Connect timeout:");
    show_help_mark(help_text, data.input_offset_x, true, "Max time (seconds) to wait for the server to accept a connection.\n\nLocal servers usually connect instantly (1-2s).");
    ImGui::SameLine();
    ImGui::SetCursorPosX(offset);
    ImGui::SetNextItemWidth(item_w);
    m_data_changed = ImGuiEx::SliderIntSmart("##ComfyUIConnTime", &buffer_i[BuffConnTM], 0, 30, "%llu sec") || m_data_changed;


    ///////////////////       read timeout
    ImGui::TextUnformatted("Read timeout:");
    show_help_mark(help_text, data.input_offset_x, true, "Max time (seconds) to wait for a response (e.g., JSON workflow).\n\nIncrease this if loading large workflows over a slow network.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(offset);
    ImGui::SetNextItemWidth(item_w);
    m_data_changed = ImGuiEx::SliderIntSmart("##ComfyUIReadTime", &buffer_i[BuffReadTM], 0, 30, "%llu sec") || m_data_changed;


    /////////////// apply
    ImGuiEx::Spacing(2);

    ImGui::SetCursorPosX(offset);
    if (!m_data_changed || buffer_s[BuffHost].empty())
        ImGui::BeginDisabled();
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, g::pOptsManager->color_load(AppColor::OptBtnNormal));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g::pOptsManager->color_load(AppColor::OptBtnHover));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g::pOptsManager->color_load(AppColor::OptBtnPressed));
    }
    const bool click = ImGui::Button("Save/Apply connection settings##applyconn", ImVec2(item_w, 0));
    if (!m_data_changed || buffer_s[BuffHost].empty())
        ImGui::EndDisabled();
    else
        ImGui::PopStyleColor(3);
    if (click)
    {
        g::pOptsManager->comfyui_host_save(buffer_s[BuffHost]);
        g::pOptsManager->comfyui_port_save(buffer_i[BuffPort]);
        g::pOptsManager->comfyui_connect_timeout_save(buffer_i[BuffConnTM]);
        g::pOptsManager->comfyui_read_timeout_save(buffer_i[BuffReadTM]);
        m_data_changed = false;

        if (!g::pFlow->get_online())
            g::pFlow->reconnect();
    }
}

void OptComfyDlg::show_interaction_opts(OptsData& data, const char*& help_text, float padding, float avail_x, float item_w, float offset)
{
    auto& buffer_s = *data.comfyui.buffer_s;

    /////////////       comfyui interaction
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), avail_x - padding, APP_COMFYUI " Server Interaction");
    ImGui::Spacing();


    /////////////       python
    ImGui::TextUnformatted("Python path:");
    show_help_mark(help_text, data.input_offset_x, true, "Change this if you have a custom Python path.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(offset);
    ImGui::SetNextItemWidth(item_w);
    ImGuiEx::InputText("##ComfyUIPython", &buffer_s[BuffPython], ImGuiInputTextFlags_AlwaysOverwrite | ImGuiInputTextFlags_EscapeClearsAll);
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->comfyui_python_save(buffer_s[BuffPython]);


    /////////////       comfyui main.py path
    ImGui::TextUnformatted("Server path:");
    show_help_mark(help_text, data.input_offset_x, true, "Full path to main.py or the " APP_COMFYUI " root directory.\n" APP_CAPTION " uses this to launch and manage the server process.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(offset);
    ImGui::SetNextItemWidth(item_w - 28.0f - ImGui::GetStyle().ItemSpacing.x);
    ImGuiEx::InputText("##ComfyUIPath", &buffer_s[BuffPath], ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g::pOptsManager->color_load(AppColor::OptDlgBg));
    if (ImGui::Button("...##ComfyUIPathSelect", ImVec2(28.0f, 0.0f)))
    {
        data.enable_always_on_top = false;
        ifd::FileDialog::Instance().Open("ComfyuiMainpyPath", APP_COMFYUI " main.py path", "main.py (main.py){main.py}", false);
    }
    if (ifd::FileDialog::Instance().IsDone("ComfyuiMainpyPath"))
    {
        if (ifd::FileDialog::Instance().HasResult())
        {
            g::pOptsManager->comfyui_main_py_save(ifd::FileDialog::Instance().GetResult());
            buffer_s[BuffPath] = g::pOptsManager->comfyui_main_py_load().u8string();
        }
        ifd::FileDialog::Instance().Close();
        data.enable_always_on_top = true;
    }
    ImGui::PopStyleColor();


    /////////////       comfyui command line
    ImGui::TextUnformatted("Launch args:");
    show_help_mark(help_text, data.input_offset_x, true, "Additional command line arguments (e.g., --cpu, --lowvram, --fast).\n\nCheck your .bat files for recommended flags.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(offset);
    ImGui::SetNextItemWidth(item_w);
    ImGuiEx::InputText("##ComfyUICmd", &buffer_s[BuffCmd], ImGuiInputTextFlags_AlwaysOverwrite | ImGuiInputTextFlags_EscapeClearsAll);
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->comfyui_command_save(buffer_s[BuffCmd]);


    ///////////// disable if no path
    //if (buffer_s[BuffPath].empty() || buffer_s[BuffPython].empty())
    //    ImGui::BeginDisabled();


    /////////////       start server
    show_help_mark(help_text, data.input_offset_x, false, "Automatically launch the " APP_COMFYUI " server when " APP_CAPTION " starts.");
    ImGui::SameLine();
    bool b_val = g::pOptsManager->comfyui_start_server_load();
    if (ImGui::Checkbox("Auto-start server##ComfyUIStart", &b_val))
    {
        g::pOptsManager->comfyui_start_server_save(b_val);
        if (!b_val)
        {
            g::pOptsManager->comfyui_kill_server_save(false);
            g::pOptsManager->comfyui_hide_console_save(false);
            g::pOptsManager->comfyui_capture_output_save(false);
            g::pOptsManager->comfyui_autohide_output_save(false);
            g::pOptsManager->comfyui_parse_address_save(false);
        }
    }


    ///////////// disable if no start
    //if (!g::pOptsManager->comfyui_start_server_load())
    //    ImGui::BeginDisabled();


    /////////////       stop server
    show_help_mark(help_text, data.input_offset_x, false, "Terminate the " APP_COMFYUI " process when closing " APP_CAPTION ".\n\nPrevents orphan python processes running in the background.");
    ImGui::SameLine();
    b_val = g::pOptsManager->comfyui_kill_server_load();
    if (ImGui::Checkbox("Auto-stop on exit##ComfyUIStop", &b_val))
    {
        g::pOptsManager->comfyui_kill_server_save(b_val);
        if (b_val)
            g::pOptsManager->comfyui_start_server_save(true);
        else
            g::pOptsManager->comfyui_hide_console_save(false);
    }


    /////////////       hide console
    show_help_mark(help_text, data.input_offset_x, false, "Hide the black command prompt window (cmd.exe).\n\nRequires 'Capture output' to be enabled to see server logs.");
    ImGui::SameLine();
    b_val = g::pOptsManager->comfyui_hide_console_load();
    if (ImGui::Checkbox("Hide console window##ComfyUIHide", &b_val))
    {
        g::pOptsManager->comfyui_hide_console_save(b_val);
        if (b_val)
        {
            g::pOptsManager->comfyui_start_server_save(true);
            g::pOptsManager->comfyui_kill_server_save(true);
            g::pOptsManager->comfyui_capture_output_save(true);
        }
    }


    /////////////       capture output
    show_help_mark(help_text, data.input_offset_x, false, "Redirect server stdout and stderr to the " APP_CAPTION " Logs panel.\n\nAllows monitoring errors and progress without the system console.");
    ImGui::SameLine();
    b_val = g::pOptsManager->comfyui_capture_output_load();
    if (ImGui::Checkbox("Capture output##ComfyUICapture", &b_val))
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


    ///////////// disable if no catch
    //if (!g::pOptsManager->comfyui_capture_output_load())
    //    ImGui::BeginDisabled();


    /////////////       auto hide output
    show_help_mark(help_text, data.input_offset_x, false, "Automatically shows the Logs panel when " APP_COMFYUI " is disconnected and hides it after a successful connection. You can always toggle the panel manually from the main menu.");
    ImGui::SameLine();
    b_val = g::pOptsManager->comfyui_autohide_output_load();
    if (ImGui::Checkbox("Autohide Logs panel##ComfyUIHideLog", &b_val))
    {
        g::pOptsManager->comfyui_autohide_output_save(b_val);
        if (b_val)
        {
            g::pOptsManager->comfyui_start_server_save(true);
            g::pOptsManager->comfyui_capture_output_save(true);
        }
    }

    /////////////       parse address
    show_help_mark(help_text, data.input_offset_x, false, "Attempt to find the listening IP and port by parsing the server startup logs.\n\nIf disabled, the manual address settings above will be used.");
    ImGui::SameLine();
    b_val = g::pOptsManager->comfyui_parse_address_load();
    if (ImGui::Checkbox("Auto-detect port##ComfyUIParse", &b_val))
    {
        g::pOptsManager->comfyui_parse_address_save(b_val);
        if (b_val)
        {
            g::pOptsManager->comfyui_start_server_save(true);
            g::pOptsManager->comfyui_capture_output_save(true);
        }
    }


    ///////////// disable if no catch
    //if (!g::pOptsManager->comfyui_capture_output_load())
    //    ImGui::EndDisabled();

    /////////////// disable if no start
    //if (!g::pOptsManager->comfyui_start_server_load())
    //    ImGui::EndDisabled();

    /////////////// disable if no path
    //if (buffer_s[BuffPath].empty() || buffer_s[BuffPython].empty())
    //    ImGui::EndDisabled();

    /////////////// apply
    ImGuiEx::Spacing(2);

    ImGui::SetCursorPosX(offset);
    if (buffer_s[BuffPath].empty() || buffer_s[BuffPython].empty())
        ImGui::BeginDisabled();
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, g::pOptsManager->color_load(AppColor::OptBtnNormal));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g::pOptsManager->color_load(AppColor::OptBtnHover));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g::pOptsManager->color_load(AppColor::OptBtnPressed));
    }
    const bool click = ImGui::Button("Start ComfyUI##start", ImVec2(item_w, 0));
    if (buffer_s[BuffPath].empty() || buffer_s[BuffPython].empty())
        ImGui::EndDisabled();
    else
        ImGui::PopStyleColor(3);
    if (click)
    {
        tools::str::trim_this(buffer_s[BuffPath]);
        tools::str::trim_this(buffer_s[BuffPython]);
        tools::str::trim_this(buffer_s[BuffCmd]);
        if (buffer_s[BuffPath].empty() || buffer_s[BuffPython].empty())
            return;
    }
}

}
