
#include "ui/dialogs/sync_projects_dlg.h"
#include "ui/graph-n_flow.h"
#include "core/managers/connect_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/serializer/wf_serializers.h"
#include "core/base/comfyui_web_paths.h"

namespace ui
{

SyncProjectsDlg::SyncProjectsDlg() noexcept
    : BaseDlgWithData(DlgObj::OptionsDlg)
{
}

SyncProjectsDlg::~SyncProjectsDlg() noexcept = default;

void SyncProjectsDlg::init()
{
    BaseDlgWithData::init();

    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    dlg_data()->x = main_viewport->WorkSize.x / 2 - 300;
    dlg_data()->y = main_viewport->WorkSize.y / 2 - 400;
    dlg_data()->width = 800;
    dlg_data()->height = 750;

    dlg_data()->clr_title = g::pOptsManager->color_load(AppColor::MiscDlgTitleBg);
    dlg_data()->clr_bg = g::pOptsManager->color_load(AppColor::MiscDlgBg);

    m_step = Step::Disconnected;
}

DlgCode SyncProjectsDlg::show(bool& /*focused*/)
{
    ImGuiEx::PushDlgTitleStyles(dlg_data()->clr_title);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, dlg_data()->clr_bg);

    ImGui::SetNextWindowPos(ImVec2(dlg_data()->x, dlg_data()->y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(dlg_data()->width, dlg_data()->height), ImGuiCond_FirstUseEver);

    bool opened = true;
    if (ImGui::Begin(DlgObjID[cast::to_szt(DlgObj::SyncProjects)], &opened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking))
    {
        load_projects();

        if (m_step != Step::Ready && m_step != Step::Sync)
            show_status_page();
        else
        {
            if (Step::Sync == m_step)
                sync();

            const bool disable_all = (m_step != Step::Ready);
            if (disable_all) ImGui::BeginDisabled();

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
                        std::ranges::sort(m_rows, [sort_spec](const SyncRow& l, const SyncRow& r) -> bool 
                        {
                            int32_t delta = 0;
                            switch (sort_spec->ColumnIndex)
                            {
                            case 1: // Project Name
                                delta = tools::str::to_lower_ascii(l.name).compare(tools::str::to_lower_ascii(r.name));
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
                                if (cast::to_i32(l.selected_action) < cast::to_i32(r.selected_action)) delta = -1;
                                else if (cast::to_i32(l.selected_action) > cast::to_i32(r.selected_action)) delta = 1;
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

                    if (SyncAction::Skip != row.selected_action)
                    {
                        ImU32 row_bg_color = 0;
                        if (SyncAction::UploadReplace == row.selected_action || SyncAction::UploadRename == row.selected_action || SyncAction::Upload == row.selected_action)
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

                    if (SyncAction::Skip == row.selected_action)
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(row.name.c_str());

                    // Format timestamps on the fly or use pre-formatted strings
                    ImGui::TableNextColumn();
                    if (row.exists_local)   ImGui::TextUnformatted(row.local_timestamp_str.c_str());
                    else                    ImGui::TextUnformatted("---");

                    ImGui::TableNextColumn();
                    if (row.exists_server)  ImGui::TextUnformatted(row.server_timestamp_str.c_str());
                    else                    ImGui::TextUnformatted("---");

                    if (SyncAction::Skip == row.selected_action)
                        ImGui::PopStyleColor();

                    const bool disable_input = (SyncStatus::InProgress == row.status);
                    if (disable_input) ImGui::BeginDisabled();

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::BeginCombo("##action", get_action_name(row.selected_action)))
                    {
                        for (auto act : row.available_actions)
                        {
                            const bool is_selected = (row.selected_action == act);
                            if (ImGui::Selectable(get_action_name(act), is_selected))
                            {
                                row.selected_action = act;

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
                    if (SyncAction::UploadRename == row.selected_action || SyncAction::DownloadRename == row.selected_action)
                    {
                        row.new_name_invalid = (SyncAction::UploadRename == row.selected_action && row.new_name == row.name);                            
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

#ifdef _DEBUG
            ImGui::SameLine();
            if (ImGui::Button("Download", ImVec2(btn_width, 0)))
            {
                for (auto& row : m_rows)
                {
                    if (row.exists_server)
                        row.selected_action = SyncAction::DownloadReplace;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Upload", ImVec2(btn_width, 0)))
            {
                for (auto& row : m_rows)
                {
                    if (row.exists_local)
                        row.selected_action = SyncAction::UploadReplace;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove", ImVec2(btn_width, 0)))
            {
                for (auto& row : m_rows)
                {
                    if (row.exists_local)
                        row.selected_action = SyncAction::DeleteLocal;
                }
            }
#endif

            ImGui::SameLine();
            const float right_cursor_x = ImGui::GetWindowWidth() - btn_width - ImGui::GetStyle().WindowPadding.x;
            ImGui::SetCursorPosX(right_cursor_x);

            if (!allow_apply) ImGui::BeginDisabled();
            if (ImGui::Button("Apply", ImVec2(btn_width, 0)))
            {
                m_current_row = 0;
                std::ranges::for_each(m_rows, [](SyncRow& row) { row.status = SyncStatus::None; });
                m_step = Step::Sync;
            }
            if (!allow_apply) ImGui::EndDisabled();

            if (disable_all) ImGui::EndDisabled();
        }
    }
    else dead();
    if (!opened) dead();
    return cleanup_and_exit(opened);
}

const char* SyncProjectsDlg::get_action_name(SyncAction action) noexcept
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
    case SyncAction::Undelete:        return "Restore (local only)";
    }
    LogRError("Unknown action", "");
}

void SyncProjectsDlg::apply_recommended_actions()
{
    for (auto& row : m_rows)
    {
        if (row.exists_local && row.deleted)
        {
            row.selected_action = SyncAction::Skip;
        }
        else if (row.exists_local && !row.exists_server)
        {
            row.selected_action = SyncAction::Upload;
        }
        else if (!row.exists_local && row.exists_server)
        {
            row.selected_action = SyncAction::Download;
        }
        else if (row.exists_local && row.exists_server)
        {
            if (row.local_timestamp > row.server_timestamp)
            {
                row.selected_action = SyncAction::UploadReplace;
            }
            else if (row.server_timestamp > row.local_timestamp)
            {
                row.selected_action = SyncAction::DownloadReplace;
            }
            else
            {
                row.selected_action = SyncAction::Skip;
            }
        }
        else
        {
            row.selected_action = SyncAction::Skip;
        }

        if ((SyncAction::UploadRename == row.selected_action || SyncAction::DownloadRename == row.selected_action) && row.new_name.empty()) 
        {
            row.new_name = row.name + " (Copy)";
        }
    }
}

void SyncProjectsDlg::load_projects()
{
    if (!g::pFlow->get_online() || !g::pFlow->get_projects_ready())
    {
        m_step = Step::Disconnected;
        return;
    }

    if (Step::Disconnected == m_step)
        m_step = Step::Loading;
    else
        return;

    m_rows.clear();
    auto wfs = g::pWfsManager->get_wfs();
    for (auto wf : wfs)
    {
        SyncRow row;
        row.local_id = wf->id;
        if (0 == row.local_id) __debugbreak();
        row.name = wf->name;
        row.exists_local = true;
        row.exists_server = false;
        row.deleted = wf->pending_delete;
        row.local_timestamp = wf->draft_exist ? wf->draft_time.time() : wf->saved_time.time();
        row.local_timestamp_str = (0 == row.local_timestamp ? "---" : wf->draft_exist ? wf->draft_time.as_full_date(false) : wf->saved_time.as_full_date(false));
        m_rows.push_back(std::move(row));
    }

    const auto all_server_names = [this](int32_t result, std::string&& payload)
    {
        if (result != HTTP_RESP_OK)
        {
            m_step = Step::LoadFailed;
            return;
        }

        std::list<std::pair<std::string, tools::dt>> projects;
        if (!data::WfDeserializerFromJson::deserialize_all_wf_names(payload, projects))
        {
            m_step = Step::LoadFailed;
            return;
        }

        for (auto& [name, time] : projects)
        {
            name = tools::str::path_to_string(std::filesystem::path(name).stem());

            bool local_found = false;
            for (auto& row : m_rows)
            {
                if (row.exists_local && !row.deleted && !row.exists_server && row.name == name)
                {
                    local_found = true;
                    row.exists_server = true;
                    row.server_timestamp = time.time();
                    row.server_timestamp_str = (0 == row.server_timestamp ? "---" : time.as_full_date(false));
                }
            }

            if (!local_found)
            {
                SyncRow row;
                row.name = name;
                row.exists_local = false;
                row.exists_server = true;
                row.server_timestamp = time.time();
                row.server_timestamp_str = (0 == row.server_timestamp ? "---" : time.as_full_date(false));
                m_rows.push_back(std::move(row));
            }
        }

        setup_actions();
        m_step = Step::Ready;
    };

    g::pFlow->http_get("Sync/Init", get_alive_callback(), comfyui_http::wfs_all_names, all_server_names);
}

void SyncProjectsDlg::setup_actions()
{
    for (auto& row : m_rows)
    {
        row.available_actions.clear();
        row.available_actions.push_back(SyncAction::Skip);

        if (row.exists_local && row.deleted)
        {
            row.available_actions.push_back(SyncAction::Undelete);
        }
        else if (row.exists_local && !row.exists_server)
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

void SyncProjectsDlg::reset() noexcept
{
    for (auto& row : m_rows)
    {
        row.selected_action = SyncAction::Skip;
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

void SyncProjectsDlg::sync()
{
    if (m_current_row >= m_rows.size())
    {
        g::pFlow->wf_reload();
        setup_actions();
        m_step = Step::Ready;
        return;
    }

    if (SyncStatus::InProgress == m_rows[m_current_row].status)
        return;

    const auto download = [this](int32_t result, std::string&& payload)
    {
        auto& row = m_rows[m_current_row++];
        Log("Downloaded project " << row.name);

        if (result != HTTP_RESP_OK)
        {
            row.status = SyncStatus::Error;
            return;
        }

        const auto project_name = (SyncAction::DownloadRename == row.selected_action ? row.new_name : row.name);
        if ((SyncAction::Download == row.selected_action || SyncAction::DownloadReplace == row.selected_action) && row.exists_local)
        {
            if (row.exists_local)
            {
                if (0 == row.local_id) __debugbreak();
                g::pFlow->wf_delete(row.local_id);
            }
        }

        row.local_id = g::pWfsManager->import_wf_from_json(payload, project_name, tools::dt::from_time(row.server_timestamp));
        if (row.local_id != 0)
        {
            g::pOptsManager->wf_imported_save(row.name + ".json");
            row.status = SyncStatus::Success;
            row.exists_local = true;
            row.local_timestamp = row.server_timestamp;
            row.local_timestamp_str = row.server_timestamp_str;
            row.selected_action = SyncAction::Skip;
        }
        else
        {
            if ((SyncAction::Download == row.selected_action || SyncAction::DownloadReplace == row.selected_action) && row.exists_local)
            {
                if (0 == row.local_id) __debugbreak();
                g::pFlow->wf_restore(row.local_id);
            }
            row.status = SyncStatus::Error;
        }
    };

    const auto upload = [this](int32_t result, std::string&&)
    {
        auto& row = m_rows[m_current_row++];
        Log("Uploaded project " << row.name);
        row.selected_action = SyncAction::Skip;
        if (row.cleanup_wf)
        {
            if (0 == row.local_id) __debugbreak();
            g::pWfsManager->access(row.local_id)->clear();
        }
        row.cleanup_wf = false;
        if (HTTP_RESP_OK == result)
        {
            row.status = SyncStatus::Success;
            row.exists_server = true;
            row.server_timestamp = tools::dt::from_current_date().time();
            row.server_timestamp_str = tools::dt::from_current_date().as_full_date(false);
        }
        else
            row.status = SyncStatus::Error;
    };

    while (m_current_row < m_rows.size())
    {
        auto& row = m_rows[m_current_row];
        switch (row.selected_action)
        {
        case SyncAction::Skip:
            {
                m_current_row++;
                row.status = SyncStatus::Skipped;
            }
            break;
        case SyncAction::DeleteLocal:
            {
                m_current_row++;
                if (0 == row.local_id) __debugbreak();
                g::pFlow->wf_delete(row.local_id);
                row.deleted = true;
                row.status = SyncStatus::Success;
                row.selected_action = SyncAction::Skip;
            }
            break;
        case SyncAction::Undelete:
            {
                m_current_row++;
                if (0 == row.local_id) __debugbreak();
                g::pFlow->wf_restore(row.local_id);
                row.deleted = false;
                row.status = SyncStatus::Success;
                row.selected_action = SyncAction::Skip;
            }
            break;
        case SyncAction::Download:
        case SyncAction::DownloadRename:
        case SyncAction::DownloadReplace:
            {
                row.status = SyncStatus::InProgress;
                const auto url = fmt::format(comfyui_http::wf_download_, tools::str::url_encode(row.name) + ".json");
                g::pFlow->http_get("Sync/Action", get_alive_callback(), url.c_str(), download);
            }
            break;
        case SyncAction::Upload:
        case SyncAction::UploadRename:
        case SyncAction::UploadReplace:
            {
                row.status = SyncStatus::InProgress;
                if (0 == row.local_id) __debugbreak();
                const bool loaded = g::pWfsManager->is_loaded(row.local_id);
                blob::Blob dummy;
                if (!loaded && !g::pWfsManager->load(row.local_id, dummy))
                {
                    row.status = SyncStatus::Error;
                }
                else
                {
                    row.cleanup_wf = !loaded;
                    if (0 == row.local_id) __debugbreak();
                    auto j = data::WfSerializerToJson(g::pWfsManager->access(row.local_id)).serialize();
                    const auto project_name = (SyncAction::UploadRename == row.selected_action ? row.new_name : row.name);
                    const auto url = fmt::format(comfyui_http::wf_upload_, tools::str::url_encode(project_name) + ".json");
                    g::pFlow->http_post("Sync/Action", get_alive_callback(), url.c_str(), j.dump(), upload);
                }
            }
            break;
        }

        if (SyncStatus::InProgress == row.status)
            break;
    }
}

}
