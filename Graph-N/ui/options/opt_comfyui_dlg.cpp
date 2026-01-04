
#include "ui/options/opt_comfyui_dlg.h"
#include "core/managers/settings_manager.h"
#include "ui/ImFileDialog.h"

namespace ui
{

void OptComfyDlg::init(OptsData& data)
{
    LogF;

    data.comfyui.host = g::pOptsManager->comfyui_get_host();
    data.comfyui.port = g::pOptsManager->comfyui_get_port();
    data.comfyui.conn_time = g::pOptsManager->comfyui_get_connect_timeout();
    data.comfyui.read_time = g::pOptsManager->comfyui_get_read_timeout();
    data.comfyui.path = g::pOptsManager->comfyui_get_main_py().u8string();
    data.comfyui.cmd = g::pOptsManager->comfyui_get_command();
    data.comfyui.start = g::pOptsManager->comfyui_get_start_server();
    data.comfyui.stop = g::pOptsManager->comfyui_get_kill_server();
    data.comfyui.hide = g::pOptsManager->comfyui_get_hide_console();
    data.comfyui.capture = g::pOptsManager->comfyui_get_capture_output();
    data.comfyui.parse = g::pOptsManager->comfyui_get_parse_address();
}

void OptComfyDlg::show(OptsData& data)
{
    const char* help_text = nullptr;

    /////////////       ComfyUI address
    ImGuiEx::Separator(data.separator_clr, APP_COMFYUI " Address");
    ImGui::Spacing();


    /////////////       host
    ImGui::TextUnformatted("Host:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGuiEx::InputText("##ComfyUIHost", &data.comfyui.host, ImGuiInputTextFlags_AutoSelectAll))
    {
        g::pOptsManager->comfyui_set_host(data.comfyui.host);
    }


    /////////////       port
    ImGui::TextUnformatted("Port:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputScalar("##ComfyUIPort", ImGuiDataType_S64, &data.comfyui.port, nullptr, nullptr, "%u",
        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
    {
        g::pOptsManager->comfyui_set_port(cast::to_u16(data.comfyui.port));
    }


    /////////////       timeouts
    ImGuiEx::Separator(data.separator_clr, APP_COMFYUI " Server Timeouts");
    ImGui::Spacing();


    /////////////       connection timeout
    ImGui::TextUnformatted("Connect Timeout:");
    show_help_mark(help_text, data.input_offset_x, true, "Max time (seconds) to wait for the server to accept a connection.\n\nLocal servers usually connect instantly (1-2s).");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputScalar("##ComfyUIConnTime", ImGuiDataType_S64, &data.comfyui.conn_time, &data.step, nullptr, "%llu",
        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
    {
        g::pOptsManager->comfyui_set_connect_timeout(data.comfyui.conn_time);
    }


    /////////////       read timeout
    ImGui::TextUnformatted("Read Timeout:");
    show_help_mark(help_text, data.input_offset_x, true, "Max time (seconds) to wait for a response (e.g., JSON workflow).\n\nIncrease this if loading large workflows over a slow network.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputScalar("##ComfyUIReadTime", ImGuiDataType_S64, &data.comfyui.read_time, &data.step, nullptr, "%llu",
        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
    {
        g::pOptsManager->comfyui_set_read_timeout(data.comfyui.read_time);
    }


    /////////////       comfyui interaction
    ImGuiEx::Separator(data.separator_clr, APP_COMFYUI " Server Interaction");
    ImGui::Spacing();


    /////////////       comfyui main.py path
    ImGui::TextUnformatted("Server Path:");
    show_help_mark(help_text, data.input_offset_x, true, "Full path to main.py or the " APP_COMFYUI " root directory.\n" APP_CAPTION " uses this to launch and manage the server process.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 28.0f - ImGui::GetStyle().ItemSpacing.x);
    ImGuiEx::InputText("##ComfyUIPath", &data.comfyui.path, ImGuiInputTextFlags_ReadOnly);
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
            data.comfyui.path = g::pOptsManager->comfyui_get_main_py().u8string();
        }
        ifd::FileDialog::Instance().Close();
    }
    ImGui::PopStyleColor();


    /////////////       comfyui command line
    ImGui::TextUnformatted("Launch Args:");
    show_help_mark(help_text, data.input_offset_x, true, "Additional command line arguments (e.g., --cpu, --lowvram, --fast).\n\nCheck your .bat files for recommended flags.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGuiEx::InputText("##ComfyUICmd", &data.comfyui.cmd, ImGuiInputTextFlags_AlwaysOverwrite | ImGuiInputTextFlags_EscapeClearsAll))
    {
        g::pOptsManager->comfyui_set_command(data.comfyui.cmd);
    }


    ///////////// disable if no path
    if (data.comfyui.path.empty() || data.comfyui.cmd.empty())
        ImGui::BeginDisabled();


    /////////////       start server
    show_help_mark(help_text, data.input_offset_x, false, "Automatically launch the " APP_COMFYUI " server when " APP_CAPTION " starts.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Auto-start server##ComfyUIStart", &data.comfyui.start))
    {
        g::pOptsManager->comfyui_set_start_server(data.comfyui.start);
    }


    ///////////// disable if no start
    if (!data.comfyui.start)
        ImGui::BeginDisabled();


    /////////////       stop server
    show_help_mark(help_text, data.input_offset_x, false, "Terminate the " APP_COMFYUI " process when closing ." APP_CAPTION ".\n\nPrevents orphan python processes running in the background.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Auto-stop on exit##ComfyUIStop", &data.comfyui.stop))
    {
        g::pOptsManager->comfyui_set_kill_server(data.comfyui.stop);
    }


    /////////////       hide console
    show_help_mark(help_text, data.input_offset_x, false, "Hide the black command prompt window (cmd.exe).\n\nRequires 'Capture Output' to be enabled to see server logs.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Hide console window##ComfyUIHide", &data.comfyui.hide))
    {
        g::pOptsManager->comfyui_set_hide_console(data.comfyui.hide);
    }


    /////////////       catch output
    show_help_mark(help_text, data.input_offset_x, false, "Redirect server stdout and stderr to the " APP_CAPTION " Logs panel.\n\nAllows monitoring errors and progress without the system console.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Capture Output##ComfyUICapture", &data.comfyui.capture))
    {
        g::pOptsManager->comfyui_set_capture_output(data.comfyui.capture);
    }


    ///////////// disable if no catch
    if (!data.comfyui.capture)
        ImGui::BeginDisabled();


    /////////////       parse address
    show_help_mark(help_text, data.input_offset_x, false, "Attempt to find the listening IP and Port by parsing the server startup logs.\n\nIf disabled, the manual address settings above will be used.");
    ImGui::SameLine();
    if (ImGui::Checkbox("Auto-Detect Port##ComfyUIParse", &data.comfyui.parse))
    {
        g::pOptsManager->comfyui_set_parse_address(data.comfyui.parse);
    }


    ///////////// disable if no catch
    if (!data.comfyui.capture)
        ImGui::EndDisabled();

    ///////////// disable if no start
    if (!data.comfyui.start)
        ImGui::EndDisabled();

    ///////////// disable if no path
    if (data.comfyui.path.empty() || data.comfyui.cmd.empty())
        ImGui::EndDisabled();


    ///////////// restart
    ImGuiEx::Separator(data.separator_clr);
    ImGui::Button("Restart " APP_COMFYUI " server");


    ///////////// help
    ImGui::Dummy(ImVec2(0.0f, 50.0f));
    show_help_text(help_text);
}

}
