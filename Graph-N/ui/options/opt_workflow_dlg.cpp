
#include "ui/options/opt_workflow_dlg.h"
#include "core/managers/settings_manager.h"

namespace ui
{

void OptWorkflowDlg::init(OptsData& /*data*/)
{
    LogF;

}

void OptWorkflowDlg::show(OptsData& /*data*/)
{
    const char* help_text = nullptr;


    ///////////////       timeouts
    //ImGui::Separator(data.separator_clr, APP_COMFYUI " server timeouts");


    ///////////////       connection timeout
    //ImGui::TextUnformatted("Connect Timeout:");
    //show_help_mark(help_text, data.input_offset_x, true, "Max time (seconds) to wait for the server to accept a connection.\n\nLocal servers usually connect instantly (1-2s).");
    //ImGui::SameLine();
    //ImGui::SetNextItemWidth(-FLT_MIN);
    //if (ImGui::InputScalar("##ComfyUIConnTime", ImGuiDataType_S64, &data.comfy.conn_time, &data.step, nullptr, "%llu",
    //    ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
    //{
    //    g::pOptsManager->comfyui_set_connect_timeout(data.comfy.conn_time);
    //}


    ImGui::Dummy(ImVec2(0.0f, 50.0f));
    show_help_text(help_text);
}

}
