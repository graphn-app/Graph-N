
#include "ui/options/opt_comfyui_dlg.h"
#include "core/managers/settings_manager.h"
#include "ui/ImFileDialog.h"
#include "ui/graph-n_flow.h"

namespace ui
{

enum
{
    buff_host,
    buff_cmd,
    buff_path,

    _buff_s_count,
};

enum
{
    buff_port,
    buff_conn_tm,
    buff_read_tm,

    _buff_i_count,
};

void OptComfyDlg::init(OptsData& data)
{
    LogF;

    data.comfyui.buffer_s->resize(_buff_s_count);
    (*data.comfyui.buffer_s)[buff_host] = g::pOptsManager->comfyui_host_load();
    (*data.comfyui.buffer_s)[buff_path] = g::pOptsManager->comfyui_main_py_load().u8string();
    (*data.comfyui.buffer_s)[buff_cmd] = g::pOptsManager->comfyui_command_load();

    data.comfyui.buffer_i->resize(_buff_i_count);
    (*data.comfyui.buffer_i)[buff_port] = g::pOptsManager->comfyui_port_load();
    (*data.comfyui.buffer_i)[buff_conn_tm] = g::pOptsManager->comfyui_connect_timeout_load();
    (*data.comfyui.buffer_i)[buff_read_tm] = g::pOptsManager->comfyui_read_timeout_load();
}

void OptComfyDlg::show(OptsData& data)
{
    const char* help_text = nullptr;
    auto& buffer_s = *data.comfyui.buffer_s;
    auto& buffer_i = *data.comfyui.buffer_i;

    /////////////       ComfyUI address
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), APP_COMFYUI " Address");
    ImGui::Spacing();


    /////////////       host
    ImGui::TextUnformatted("Host:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::InputText("##ComfyUIHost", &buffer_s[buff_host], ImGuiInputTextFlags_AutoSelectAll);


    /////////////       port
    ImGui::TextUnformatted("Port:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::SliderIntSmart("##ComfyUIPort", &buffer_i[buff_port], 1, 65535, "%u");


    /////////////       timeouts
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), APP_COMFYUI " Server Timeouts");
    ImGui::Spacing();


    /////////////       connection timeout
    ImGui::TextUnformatted("Connect Timeout:");
    show_help_mark(help_text, data.input_offset_x, true, "Max time (seconds) to wait for the server to accept a connection.\n\nLocal servers usually connect instantly (1-2s).");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::SliderIntSmart("##ComfyUIConnTime", &buffer_i[buff_conn_tm], 0, 30, "%llu sec");


    /////////////       read timeout
    ImGui::TextUnformatted("Read Timeout:");
    show_help_mark(help_text, data.input_offset_x, true, "Max time (seconds) to wait for a response (e.g., JSON workflow).\n\nIncrease this if loading large workflows over a slow network.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::SliderIntSmart("##ComfyUIReadTime", &buffer_i[buff_read_tm], 0, 30, "%llu sec");

    /////////////       comfyui interaction
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), APP_COMFYUI " Server Interaction");
    ImGui::Spacing();


    /////////////       comfyui main.py path
    ImGui::TextUnformatted("Server Path:");
    show_help_mark(help_text, data.input_offset_x, true, "Full path to main.py or the " APP_COMFYUI " root directory.\n" APP_CAPTION " uses this to launch and manage the server process.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 28.0f - ImGui::GetStyle().ItemSpacing.x);
    ImGuiEx::InputText("##ComfyUIPath", &buffer_s[buff_path], ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g::pOptsManager->color_load(AppColor::opt_dlg_bg));
    if (ImGui::Button("...##ComfyUIPathSelect", ImVec2(28.0f, 0.0f)))
    {
        ifd::FileDialog::Instance().Open("ComfyuiMainpyPath", APP_COMFYUI " main.py path", "main.py (main.py){main.py}", false);
    }
    if (ifd::FileDialog::Instance().IsDone("ComfyuiMainpyPath"))
    {
        if (ifd::FileDialog::Instance().HasResult())
        {
            g::pOptsManager->comfyui_main_py_save(ifd::FileDialog::Instance().GetResult());
            buffer_s[buff_path] = g::pOptsManager->comfyui_main_py_load().u8string();
        }
        ifd::FileDialog::Instance().Close();
    }
    ImGui::PopStyleColor();


    /////////////       comfyui command line
    ImGui::TextUnformatted("Launch Args:");
    show_help_mark(help_text, data.input_offset_x, true, "Additional command line arguments (e.g., --cpu, --lowvram, --fast).\n\nCheck your .bat files for recommended flags.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::InputText("##ComfyUICmd", &buffer_s[buff_cmd], ImGuiInputTextFlags_AlwaysOverwrite | ImGuiInputTextFlags_EscapeClearsAll);
    if (ImGui::IsItemDeactivatedAfterEdit())
        g::pOptsManager->comfyui_command_save(buffer_s[buff_cmd]);


    ///////////// disable if no path
    if (buffer_s[buff_path].empty())
        ImGui::BeginDisabled();


    /////////////       start server
    show_help_mark(help_text, data.input_offset_x, false, "Automatically launch the " APP_COMFYUI " server when " APP_CAPTION " starts.");
    ImGui::SameLine();
    bool b_val = g::pOptsManager->comfyui_start_server_load();
    if (ImGui::Checkbox("Auto-start server##ComfyUIStart", &b_val))
        g::pOptsManager->comfyui_start_server_save(b_val);


    ///////////// disable if no start
    if (!g::pOptsManager->comfyui_start_server_load())
        ImGui::BeginDisabled();


    /////////////       stop server
    show_help_mark(help_text, data.input_offset_x, false, "Terminate the " APP_COMFYUI " process when closing ." APP_CAPTION ".\n\nPrevents orphan python processes running in the background.");
    ImGui::SameLine();
    b_val = g::pOptsManager->comfyui_kill_server_load();
    if (ImGui::Checkbox("Auto-stop on exit##ComfyUIStop", &b_val))
        g::pOptsManager->comfyui_kill_server_save(b_val);


    /////////////       hide console
    show_help_mark(help_text, data.input_offset_x, false, "Hide the black command prompt window (cmd.exe).\n\nRequires 'Capture Output' to be enabled to see server logs.");
    ImGui::SameLine();
    b_val = g::pOptsManager->comfyui_hide_console_load();
    if (ImGui::Checkbox("Hide console window##ComfyUIHide", &b_val))
        g::pOptsManager->comfyui_hide_console_save(b_val);


    /////////////       capture output
    show_help_mark(help_text, data.input_offset_x, false, "Redirect server stdout and stderr to the " APP_CAPTION " Logs panel.\n\nAllows monitoring errors and progress without the system console.");
    ImGui::SameLine();
    b_val = g::pOptsManager->comfyui_capture_output_load();
    if (ImGui::Checkbox("Capture Output##ComfyUICapture", &b_val))
        g::pOptsManager->comfyui_capture_output_save(b_val);


    ///////////// disable if no catch
    if (!g::pOptsManager->comfyui_capture_output_load())
        ImGui::BeginDisabled();


    /////////////       parse address
    show_help_mark(help_text, data.input_offset_x, false, "Attempt to find the listening IP and Port by parsing the server startup logs.\n\nIf disabled, the manual address settings above will be used.");
    ImGui::SameLine();
    b_val = g::pOptsManager->comfyui_get_parse_address_load();
    if (ImGui::Checkbox("Auto-Detect Port##ComfyUIParse", &b_val))
        g::pOptsManager->comfyui_parse_address_save(b_val);


    ///////////// disable if no catch
    if (!g::pOptsManager->comfyui_capture_output_load())
        ImGui::EndDisabled();

    ///////////// disable if no start
    if (!g::pOptsManager->comfyui_start_server_load())
        ImGui::EndDisabled();

    ///////////// disable if no path
    if (buffer_s[buff_path].empty())
        ImGui::EndDisabled();


    ///////////// apply connection settings
    ImGuiEx::Spacing(2);
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator));
    if (ImGui::Button("Save/Apply connection settings##applyconn"))
    {
        g::pOptsManager->comfyui_host_save(buffer_s[buff_host]);
        g::pOptsManager->comfyui_port_save(buffer_i[buff_port]);
        g::pOptsManager->comfyui_connect_timeout_save(buffer_i[buff_conn_tm]);
        g::pOptsManager->comfyui_read_timeout_save(buffer_i[buff_read_tm]);

        if (!g::pFlow->get_online())
            g::pFlow->reconnect();
    }


    ///////////// help
    ImGui::Dummy(ImVec2(0.0f, 50.0f));
    show_help_text(help_text);
}

}
