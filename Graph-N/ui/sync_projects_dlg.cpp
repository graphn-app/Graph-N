
#include "ui/sync_projects_dlg.h"

namespace ui
{

static std::string format_time(time_t t) 
{
    if (t == 0)
        return "Missing";

    std::tm* tm_info = std::localtime(&t);
    return fmt::format("{:%Y-%m-%d %H:%M}", *tm_info);
}

SyncProjectsDlg::SyncProjectsDlg()
    : BaseDlgWithData(DlgObj::OptionsDlg)
{
}

SyncProjectsDlg::~SyncProjectsDlg()
{
    LogF;
}

void SyncProjectsDlg::init()
{
    LogF;

    BaseDlgWithData::init();

    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    data()->x = main_viewport->WorkSize.x / 2 - 300;
    data()->y = main_viewport->WorkSize.y / 2 - 400;
    data()->width = 800;
    data()->height = 750;

    data()->clr_title = g::pOptsManager->color_load(AppColor::MiscDlgTitleBg);
    data()->clr_bg = g::pOptsManager->color_load(AppColor::MiscDlgBg);
}

DlgCode SyncProjectsDlg::show(bool& /*focused*/)
{
    ImGuiEx::PushDlgTitleStyles(data()->clr_title);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, data()->clr_bg);

    ImGui::SetNextWindowPos(ImVec2(data()->x, data()->y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(data()->width, data()->height), ImGuiCond_Always);

    bool opened = true;
    if (ImGui::Begin(DlgObjID[cast::to_szt(DlgObj::SyncProjects)], &opened, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking))
    {
        if (ImGui::BeginTable("SyncTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
            ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) 
        {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 24.0f); 
            ImGui::TableSetupColumn("Project", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("Local modified", ImGuiTableColumnFlags_WidthStretch, 1.5f);
            ImGui::TableSetupColumn("Server modified", ImGuiTableColumnFlags_WidthStretch, 1.5f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("New name", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableHeadersRow();

            for (auto& row : m_rows) 
            {
                ImGui::PushID(row.id.c_str());
                ImGui::TableNextRow();

                bool is_skipped = (row.current_action == SyncAction::Skip);

                if (!is_skipped) 
                {
                    ImU32 row_bg_color;
                    if (row.current_action == SyncAction::UploadReplace || row.current_action == SyncAction::UploadCopy) 
                        row_bg_color = IM_COL32(46, 204, 113, 30);
                    else 
                        row_bg_color = IM_COL32(52, 152, 219, 30);

                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_bg_color);
                }

                ImGui::TableNextColumn();
                switch (row.status) 
                {
                case SyncStatus::InProgress:
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(52, 152, 219, 255)), ICON_LC_LOADER);
                    break;
                case SyncStatus::Success:
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(46, 204, 113, 255)), ICON_LC_CHECK);
                    break;
                case SyncStatus::Error:
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(231, 76, 60, 255)), ICON_LC_X);
                    break;
                case SyncStatus::Skipped:
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(149, 165, 166, 255)), ICON_LC_MINUS);
                    break;
                case SyncStatus::None:
                default:
                    ImGui::TextUnformatted(" ");
                    break;
                }

                if (is_skipped)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(IM_COL32(150, 150, 150, 255)));

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(row.name.c_str());

                // Format timestamps on the fly or use pre-formatted strings
                ImGui::TableNextColumn();
                std::string local_txt = row.exists_local ? format_time(row.local_timestamp) : "Missing";
                ImGui::TextUnformatted(local_txt.c_str());

                ImGui::TableNextColumn();
                std::string server_txt = row.exists_server ? format_time(row.server_timestamp) : "Missing";
                ImGui::TextUnformatted(server_txt.c_str());

                if (is_skipped) ImGui::PopStyleColor();

                bool disable_input = (row.status == SyncStatus::InProgress || row.status == SyncStatus::Success);
                if (disable_input) ImGui::BeginDisabled();

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::BeginCombo("##action", get_action_name(row.current_action))) 
                {
                    for (auto act : row.available_actions) 
                    {
                        bool is_selected = (row.current_action == act);
                        if (ImGui::Selectable(get_action_name(act), is_selected)) 
                        {
                            row.current_action = act;

                            if ((act == SyncAction::UploadCopy || act == SyncAction::DownloadCopy) && row.new_name.empty()) 
                            {
                                row.new_name = row.name + " (Copy)";
                            }
                        }
                        if (is_selected) 
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::TableNextColumn();
                if (row.current_action == SyncAction::UploadCopy || row.current_action == SyncAction::DownloadCopy) 
                {
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGuiEx::InputText("##newname", &row.new_name, sizeof(row.new_name));
                } 
                else 
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(IM_COL32(128, 128, 128, 255)));
                    ImGui::TextUnformatted("-");
                    ImGui::PopStyleColor();
                }

                if (disable_input) ImGui::EndDisabled();

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    return cleanup_and_exit(opened);
}

const char* SyncProjectsDlg::get_action_name(SyncAction action) 
{
    switch (action) 
    {
    case SyncAction::Skip:            return "Skip";
    case SyncAction::UploadReplace:   return "Upload (replace)";
    case SyncAction::UploadCopy:      return "Upload as copy";
    case SyncAction::DownloadReplace: return "Download (replace)";
    case SyncAction::DownloadCopy:    return "Download as copy";
    }
    LogAndThrowInternalError(ExcepCategory::Logic, "Unknown action");
}

void SyncProjectsDlg::apply_recommended_actions()
{
    for (auto& row : m_rows)
    {
        if (row.status == SyncStatus::InProgress || row.status == SyncStatus::Success)
            continue;

        if (row.exists_local && !row.exists_server)
        {
            row.current_action = SyncAction::UploadReplace;
        }
        else if (!row.exists_local && row.exists_server)
        {
            row.current_action = SyncAction::DownloadReplace;
        }
        else if (row.exists_local && row.exists_server)
        {
            if (row.local_timestamp > row.server_timestamp)
            {
                row.current_action = SyncAction::UploadReplace;
            }
            else if (row.server_timestamp > row.local_timestamp)
            {
                row.current_action = SyncAction::DownloadReplace;
            }
            else
            {
                row.current_action = SyncAction::Skip;
            }
        }
        else
        {
            row.current_action = SyncAction::Skip;
        }

        if ((row.current_action == SyncAction::UploadCopy || row.current_action == SyncAction::DownloadCopy) && row.new_name.empty()) 
        {
            row.new_name = row.name + " (Copy)";
        }
    }
}

}
