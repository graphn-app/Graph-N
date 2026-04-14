
#include "ui/sync_projects_dlg.h"
#include "ui/graph-n_flow.h"
#include "core/managers/connect_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/workflow_serializer_to_json.h"

namespace ui
{

SyncProjectsDlg::SyncProjectsDlg()
    : BaseDlgWithData(DlgObj::OptionsDlg)
{
    m_alive = std::make_shared<bool>();
}

SyncProjectsDlg::~SyncProjectsDlg()
{
    LogF;
    *m_alive = false;
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

    *m_alive = true;
    m_step = Step::Disconnected;
}

DlgCode SyncProjectsDlg::show(bool& /*focused*/)
{
    ImGuiEx::PushDlgTitleStyles(data()->clr_title);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, data()->clr_bg);

    ImGui::SetNextWindowPos(ImVec2(data()->x, data()->y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(data()->width, data()->height), ImGuiCond_FirstUseEver);

    bool opened = true;
    if (ImGui::Begin(DlgObjID[cast::to_szt(DlgObj::SyncProjects)], &opened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking))
    {
        load_projects();

        if (m_step != Step::Ready && m_step != Step::Sync)
            show_status_page();
        else
        {
            if (m_step != Step::Ready) ImGui::BeginDisabled();
            const float table_height = ImGui::GetContentRegionAvail().y - 36.0f;
            const ImVec2 outer_size = ImVec2(0.0f, table_height);
            bool allow_apply = true;

            if (ImGui::BeginTable("SyncTable", 6,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable,
                outer_size))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 24.0f);
                ImGui::TableSetupColumn("Project", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                ImGui::TableSetupColumn("Local modified", ImGuiTableColumnFlags_WidthStretch, 1.5f);
                ImGui::TableSetupColumn("Server modified", ImGuiTableColumnFlags_WidthStretch, 1.5f);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                ImGui::TableSetupColumn("New name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort, 2.0f);
                ImGui::TableHeadersRow();

                if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
                {
                    if (sorts_specs->SpecsDirty)
                    {
                        const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[0];
                        std::sort(m_rows.begin(), m_rows.end(), [sort_spec](const SyncRow& l, const SyncRow& r) -> bool 
                        {
                            int32_t delta = 0;
                            switch (sort_spec->ColumnIndex)
                            {
                            case 1: // Project Name
                                delta = l.name.compare(r.name);
                                break;
                            case 2: // Local Modified
                                if (l.local_timestamp < r.local_timestamp) delta = -1;
                                else if (l.local_timestamp > r.local_timestamp) delta = 1;
                                break;
                            case 3: // Server Modified
                                if (l.server_timestamp < r.server_timestamp) delta = -1;
                                else if (l.server_timestamp > r.server_timestamp) delta = 1;
                                break;
                            case 4: // Action
                                if (cast::to_i32(l.current_action) < cast::to_i32(r.current_action)) delta = -1;
                                else if (cast::to_i32(l.current_action) > cast::to_i32(r.current_action)) delta = 1;
                                break;
                            default:
                                break;
                            }

                            if (delta > 0)
                                return ImGuiSortDirection_Ascending == sort_spec->SortDirection ? false : true;
                            if (delta < 0)
                                return ImGuiSortDirection_Ascending == sort_spec->SortDirection ? true : false;
                            return false; // equal
                        });

                        sorts_specs->SpecsDirty = false;
                    }
                }

                for (size_t idx = 0; idx < m_rows.size(); idx++)
                {
                    auto& row = m_rows[idx];
                    ImGui::PushID(cast::to_i32(idx));
                    ImGui::TableNextRow();

                    if (SyncAction::Skip != row.current_action)
                    {
                        ImU32 row_bg_color = 0;
                        if (SyncAction::UploadReplace == row.current_action || SyncAction::UploadRename == row.current_action || SyncAction::Upload == row.current_action)
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

                    if (SyncAction::Skip == row.current_action)
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(row.name.c_str());

                    // Format timestamps on the fly or use pre-formatted strings
                    ImGui::TableNextColumn();
                    if (row.exists_local)   ImGui::TextUnformatted(row.local_timestamp_str.c_str());
                    else                    ImGui::TextUnformatted("Missing");

                    ImGui::TableNextColumn();
                    if (row.exists_server)  ImGui::TextUnformatted(row.server_timestamp_str.c_str());
                    else                    ImGui::TextUnformatted("Missing");

                    if (SyncAction::Skip == row.current_action)
                        ImGui::PopStyleColor();

                    const bool disable_input = (SyncStatus::InProgress == row.status || SyncStatus::Success == row.status);
                    if (disable_input) ImGui::BeginDisabled();

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::BeginCombo("##action", get_action_name(row.current_action)))
                    {
                        for (auto act : row.available_actions)
                        {
                            const bool is_selected = (row.current_action == act);
                            if (ImGui::Selectable(get_action_name(act), is_selected))
                            {
                                row.current_action = act;

                                if ((SyncAction::UploadRename == act || SyncAction::DownloadRename == act) && row.new_name.empty())
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
                    if (SyncAction::UploadRename == row.current_action || SyncAction::DownloadRename == row.current_action)
                    {
                        row.new_name_invalid = (SyncAction::UploadRename == row.current_action && row.new_name == row.name);                            
                        if (row.new_name_invalid)
                        {
                            allow_apply = false;
                            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(150, 0, 0, 255));
                        }
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGuiEx::InputText("##newname", &row.new_name);
                        if (row.new_name_invalid) ImGui::PopStyleColor();
                    }

                    if (disable_input) ImGui::EndDisabled();

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }

            constexpr float btn_width = 120.0f;
            
            if (ImGui::Button("Recommended", ImVec2(btn_width, 0)))
            {
                apply_recommended_actions();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset", ImVec2(btn_width, 0)))
            {
                reset();
            }
            ImGui::SameLine();
            const float right_cursor_x = ImGui::GetWindowWidth() - btn_width - ImGui::GetStyle().WindowPadding.x;
            ImGui::SetCursorPosX(right_cursor_x);
            if (!allow_apply) ImGui::BeginDisabled();
            if (ImGui::Button("Apply", ImVec2(btn_width, 0)))
            {
                apply();
            }
            if (!allow_apply) ImGui::EndDisabled();

            if (m_step != Step::Ready) ImGui::EndDisabled();
        }
    }
    else *m_alive = false;
    if (!opened) *m_alive = false;
    return cleanup_and_exit(opened);
}

const char* SyncProjectsDlg::get_action_name(SyncAction action) 
{
    switch (action) 
    {
    case SyncAction::Skip:            return "Skip";
    case SyncAction::Upload:          return "Upload";
    case SyncAction::UploadReplace:   return "Upload (replace)";
    case SyncAction::UploadRename:    return "Upload (rename)";
    case SyncAction::Download:        return "Download";
    case SyncAction::DownloadReplace: return "Download (replace)";
    case SyncAction::DownloadRename:  return "Download (rename)";
    case SyncAction::DeleteLocal:     return "Remove (local only)";
    }
    LogAndThrowInternalError(ExcepCategory::Logic, "Unknown action");
}

void SyncProjectsDlg::apply_recommended_actions()
{
    for (auto& row : m_rows)
    {
        if (SyncStatus::InProgress == row.status)
            continue;
        if (SyncStatus::Success == row.status)
        {
            row.current_action = SyncAction::Skip;
            continue;
        }

        if (row.exists_local && !row.exists_server)
        {
            row.current_action = SyncAction::Upload;
        }
        else if (!row.exists_local && row.exists_server)
        {
            row.current_action = SyncAction::Download;
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

        if ((SyncAction::UploadRename == row.current_action || SyncAction::DownloadRename == row.current_action) && row.new_name.empty()) 
        {
            row.new_name = row.name + " (Copy)";
        }
    }
}

void SyncProjectsDlg::load_projects()
{
    if (!g::pFlow->get_online() || !g::pFlow->get_nodes_ready() || !g::pFlow->get_projects_imported())
    {
        m_step = Step::Disconnected;
        return;
    }

    if (Step::Disconnected == m_step)
        m_step = Step::Loading;
    else
        return;

    m_rows.clear();
    auto wfs = g::pWfsManager->get_undeleted();
    for (auto wf : wfs)
    {
        SyncRow row;
        row.local_id = wf->id;
        row.name = wf->name;
        row.exists_local = true;
        row.exists_server = false;
        row.local_timestamp = wf->draft_exist ? wf->draft_time.time() : wf->saved_time.time();
        row.local_timestamp_str = (0 == row.local_timestamp ? "Missing" : wf->draft_exist ? wf->draft_time.as_full_date(false) : wf->saved_time.as_full_date(false));
        m_rows.push_back(std::move(row));
    }

    const auto all_server_names = [this, is_alive = m_alive](bool result, std::string&& payload)
    {
        if (!*is_alive) return;
        if (!result)
        {
            m_step = Step::LoadFailed;
            return;
        }

        std::list<std::pair<std::string, tools::dt>> projects;
        if (!data::WorkflowSerializerToJson::deserialize_all_wf_names(payload, projects))
        {
            m_step = Step::LoadFailed;
            return;
        }

        for (auto& [name, time] : projects)
        {
            std::filesystem::path path(name);
            name = path.replace_extension().filename().u8string();

            bool local_found = false;
            for (auto& row : m_rows)
            {
                if (row.exists_local && !row.exists_server && row.name == name)
                {
                    local_found = true;
                    row.exists_server = true;
                    row.server_timestamp = time.time();
                    row.server_timestamp_str = (0 == row.server_timestamp ? "Missing" : time.as_full_date(false));
                }
            }

            if (!local_found)
            {
                SyncRow row;
                row.name = name;
                row.exists_local = false;
                row.exists_server = true;
                row.server_timestamp = time.time();
                row.server_timestamp_str = (0 == row.server_timestamp ? "Missing" : time.as_full_date(false));
                m_rows.push_back(std::move(row));
            }
        }

        setup_actions();
        m_step = Step::Ready;
    };

    g::pConnectManager->send_http_request(comfyui_http::wfs_all_names, false, all_server_names);
}

void SyncProjectsDlg::setup_actions()
{
    for (auto& row : m_rows)
    {
        row.current_action = SyncAction::Skip;
        row.available_actions.push_back(SyncAction::Skip);

        if (row.exists_local && !row.exists_server)
        {
            row.available_actions.push_back(SyncAction::Upload);
            row.available_actions.push_back(SyncAction::UploadRename);
            row.available_actions.push_back(SyncAction::DeleteLocal);
        }
        else if (!row.exists_local && row.exists_server)
        {
            row.available_actions.push_back(SyncAction::Download);
            row.available_actions.push_back(SyncAction::DownloadRename);
        }
        else if (row.exists_local && row.exists_server)
        {
            row.available_actions.push_back(SyncAction::UploadRename);
            row.available_actions.push_back(SyncAction::UploadReplace);
            row.available_actions.push_back(SyncAction::DownloadRename);
            row.available_actions.push_back(SyncAction::DownloadReplace);
            row.available_actions.push_back(SyncAction::DeleteLocal);
        }
    }

    apply_recommended_actions();
}

void SyncProjectsDlg::reset()
{
    for (auto& row : m_rows)
    {
        row.current_action = SyncAction::Skip;
    }
}

void SyncProjectsDlg::show_status_page()
{
    const float line_h = ImGui::GetTextLineHeight();
    const float spacing = ImGui::GetStyle().ItemSpacing.y;
    const float total_h = line_h + spacing * 4;
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    float start_y = (avail.y - total_h) * 0.5f;
    if (start_y < 20.0f)
        start_y = 20.0f; // Safety margin

    ImGui::Spacing();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + start_y);

    const char* text = nullptr;
    switch (m_step)
    {
    case Step::Disconnected:
    case Step::Loading:
        text = ICON_LC_LOADER " Loading projects";
        break;
    case Step::LoadFailed:
        text = ICON_LC_CIRCLE_SLASH " Load failed";
        break;
    default:
        text = ICON_LC_BUG " Something went wrong";
    }

    const float w = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((avail.x - w) * 0.5f);
    ImGui::TextUnformatted(text);
}

void SyncProjectsDlg::apply()
{
    if (Step::Ready == m_step)
    {
        m_step = Step::Sync;
    }
    else if (Step::Sync == m_step)
    {
    }    
}

}
