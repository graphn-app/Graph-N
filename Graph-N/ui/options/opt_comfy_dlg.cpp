
#include "ui/options/opt_comfy_dlg.h"
#include "core/managers/settings_manager.h"
#include "ui/ImFileDialog.h"

namespace ui
{

void OptComfyDlg::init(OptsData& data)
{
    LogF;

    data.comfy.host = g::pOptsManager->comfyui_get_host();
    data.comfy.port = g::pOptsManager->comfyui_get_port();
    data.comfy.conn_time = g::pOptsManager->comfyui_get_connect_timeout();
    data.comfy.read_time = g::pOptsManager->comfyui_get_read_timeout();
    data.comfy.path = g::pOptsManager->comfyui_get_main_py().u8string();
    data.comfy.cmd = g::pOptsManager->comfyui_get_command();
    data.comfy.start = g::pOptsManager->comfyui_get_start_server();
    data.comfy.stop = g::pOptsManager->comfyui_get_kill_server();
    data.comfy.hide = g::pOptsManager->comfyui_get_hide_console();
    data.comfy.capture = g::pOptsManager->comfyui_get_capture_output();
    data.comfy.parse = g::pOptsManager->comfyui_get_parse_address();

    data.help_marks.resize(10);
}

void OptComfyDlg::show(OptsData& data)
{
    const char* help_text = nullptr;

    /////////////       ComfyUI address
    ImGui::Separator(data.separator_clr, APP_COMFYUI " address");


    /////////////       host
    ImGui::TextUnformatted("Host:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputText("##ComfyUIHost", &data.comfy.host, ImGuiInputTextFlags_AutoSelectAll))
    {
        g::pOptsManager->comfyui_set_host(data.comfy.host);
    }


    /////////////       port
    ImGui::TextUnformatted("Port:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputScalar("##ComfyUIPort", ImGuiDataType_S64, &data.comfy.port, nullptr, nullptr, "%u",
        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
    {
        g::pOptsManager->comfyui_set_port(cast::to_u16(data.comfy.port));
    }


    /////////////       timeouts
    ImGui::Separator(data.separator_clr, APP_COMFYUI " server timeouts");


    /////////////       connection timeout
    ImGui::TextUnformatted("Connect Timeout:");
    show_help_mark(help_text, data.input_offset_x, true, "Max time (seconds) to wait for the server to accept a connection.\n\nLocal servers usually connect instantly (1-2s).");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputScalar("##ComfyUIConnTime", ImGuiDataType_S64, &data.comfy.conn_time, &data.step, nullptr, "%llu",
        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
    {
        g::pOptsManager->comfyui_set_connect_timeout(data.comfy.conn_time);
    }


    /////////////       read timeout
    ImGui::TextUnformatted("Read Timeout:");
    show_help_mark(help_text, data.input_offset_x, true, "Max time (seconds) to wait for a response (e.g., JSON workflow).\n\nIncrease this if loading large workflows over a slow network.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputScalar("##ComfyUIReadTime", ImGuiDataType_S64, &data.comfy.read_time, &data.step, nullptr, "%llu",
        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
    {
        g::pOptsManager->comfyui_set_read_timeout(data.comfy.read_time);
    }


    /////////////       comfyui interaction
    ImGui::Separator(data.separator_clr, APP_COMFYUI " Server Interaction");


    /////////////       comfyui main.py path
    ImGui::TextUnformatted("Server Path:");
    show_help_mark(help_text, data.input_offset_x, true, "Full path to main.py or the " APP_COMFYUI " root directory.\n" APP_CAPTION " uses this to launch and manage the server process.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 28.0f - ImGui::GetStyle().ItemSpacing.x);
    ImGui::InputText("##ComfyUIPath", &data.comfy.path, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(0, 0, 0, 255));
    if (ImGui::Button("...##ComfyUIPathSelect", ImVec2(28.0f, 0.0f)))
    {
        ifd::FileDialog::Instance().Open("ComfyuiMainpyPath", APP_COMFYUI " main.py path", "main.py (main.py){main.py}", false);
    }
    if (ifd::FileDialog::Instance().IsDone("ComfyuiMainpyPath"))
    {
        if (ifd::FileDialog::Instance().HasResult())
        {
            g::pOptsManager->comfyui_set_main_py(ifd::FileDialog::Instance().GetResult());
            data.comfy.path = g::pOptsManager->comfyui_get_main_py().u8string();
        }
        ifd::FileDialog::Instance().Close();
    }
    ImGui::PopStyleColor();


    /////////////       comfyui command line
    ImGui::TextUnformatted("Launch Args:");
    show_help_mark(help_text, data.input_offset_x, true, "Additional command line arguments (e.g., --cpu, --lowvram, --fast).\n\nCheck your .bat files for recommended flags.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputText("##ComfyUICmd", &data.comfy.cmd, ImGuiInputTextFlags_AlwaysOverwrite | ImGuiInputTextFlags_EscapeClearsAll))
    {
        g::pOptsManager->comfyui_set_command(data.comfy.cmd);
    }


    ///////////// disable if no path
    if (data.comfy.path.empty() || data.comfy.cmd.empty())
        ImGui::BeginDisabled();


    /////////////       start server
    show_help_mark(help_text, data.input_offset_x, false, "Automatically launch the " APP_COMFYUI " server when " APP_CAPTION " starts.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Auto-start server##ComfyUIStart", &data.comfy.start))
    {
        g::pOptsManager->comfyui_set_start_server(data.comfy.start);
    }


    ///////////// disable if no start
    if (!data.comfy.start)
        ImGui::BeginDisabled();


    /////////////       stop server
    show_help_mark(help_text, data.input_offset_x, false, "Terminate the " APP_COMFYUI " process when closing ." APP_CAPTION ".\n\nPrevents orphan python processes running in the background.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Auto-stop on exit##ComfyUIStop", &data.comfy.stop))
    {
        g::pOptsManager->comfyui_set_kill_server(data.comfy.stop);
    }


    /////////////       hide console
    show_help_mark(help_text, data.input_offset_x, false, "Hide the black command prompt window (cmd.exe).\n\nRequires 'Capture Output' to be enabled to see server logs.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Hide console window##ComfyUIHide", &data.comfy.hide))
    {
        g::pOptsManager->comfyui_set_hide_console(data.comfy.hide);
    }


    /////////////       catch output
    show_help_mark(help_text, data.input_offset_x, false, "Redirect server stdout and stderr to the " APP_CAPTION " Logs panel.\n\nAllows monitoring errors and progress without the system console.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Capture Output##ComfyUICapture", &data.comfy.capture))
    {
        g::pOptsManager->comfyui_set_capture_output(data.comfy.capture);
    }


    ///////////// disable if no catch
    if (!data.comfy.capture)
        ImGui::BeginDisabled();


    /////////////       parse address
    show_help_mark(help_text, data.input_offset_x, false, "Attempt to find the listening IP and Port by parsing the server startup logs.\n\nIf disabled, the manual address settings above will be used.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Auto-Detect Port##ComfyUIParse", &data.comfy.parse))
    {
        g::pOptsManager->comfyui_set_parse_address(data.comfy.parse);
    }


    ///////////// disable if no catch
    if (!data.comfy.capture)
        ImGui::EndDisabled();

    ///////////// disable if no start
    if (!data.comfy.start)
        ImGui::EndDisabled();

    ///////////// disable if no path
    if (data.comfy.path.empty() || data.comfy.cmd.empty())
        ImGui::EndDisabled();


    ///////////// restart
    ImGui::Separator(data.separator_clr);
    ImGui::Button("Restart " APP_COMFYUI " server");

    ImGui::Dummy(ImVec2(0.0f, 50.0f));
    show_help_text(help_text);
}

}
