/*
*   Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/dialogs/about_dlg.h"
#include "ui/graph-n_flow.h"
#include "ui/helpers/textures.h"
#include "core/base/version.h"
#include "main.h"

namespace ui
{

AboutDlg::AboutDlg(bool autocheck) noexcept
    : BaseDlgWithData(DlgObj::AboutDlg)
    , m_autocheck(autocheck)
{
}

AboutDlg::~AboutDlg() noexcept = default;

void AboutDlg::init()
{
    BaseDlgWithData::init();

    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    dlg_data()->x = main_viewport->WorkSize.x / 2 - 300;
    dlg_data()->y = main_viewport->WorkSize.y / 2 - 400;
    dlg_data()->width = 800;
    dlg_data()->height = 500;

    dlg_data()->clr_title = g::pOptsManager->color_load(AppColor::MiscDlgTitleBg);
    dlg_data()->clr_bg = g::pOptsManager->color_load(AppColor::MiscDlgBg);

    m_ico_path = fs::get_app_icon_path(true);
    m_ico_id = g::pTextures->load_image_from_path(m_ico_path, ABOUT_ICON_RECT, ABOUT_ICON_RECT, ABOUT_ICON_LIFETIME);

    m_version = ver::ver_as_caption();
    m_release_date = fmt::format("{:02} {}, {}", ver::release_day, tools::dt::month_name(ver::release_month, 5), ver::release_year);

    std::filesystem::path license, third_party, privacy, readme;
    fs::get_license_paths(license, third_party, privacy, readme);

    m_license = tools::path::read_file(license, false);
    m_third_party = tools::path::read_file(third_party, false);
    m_privacy = tools::path::read_file(privacy, false);
    m_readme = tools::path::read_file(readme, false);

    std::string v, d;
    if (g::pFlow->get_updates_info(v, d))
    {
        m_http_result = m_upd_checked = m_upd_found = true;
        m_new_version = fmt::format(FMT_STRING("{} ({})"), v, d);
    }
}

DlgCode AboutDlg::show(bool& /*focused*/)
{
    ImGuiEx::PushDlgTitleStyles(dlg_data()->clr_title);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, dlg_data()->clr_bg);

    ImGui::SetNextWindowPos(ImVec2(dlg_data()->x, dlg_data()->y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(dlg_data()->width, dlg_data()->height), ImGuiCond_Always);

    const auto upd_callback = [this](bool http_result, bool upd_found, const std::string& version, const std::string& date)
    {
        m_upd_checked = true;
        m_upd_checking = false;
        m_http_result = http_result;
        m_upd_found = upd_found;
        m_new_version = fmt::format(FMT_STRING("{} ({})"), version, date);
    };

    bool opened = true;
    if (ImGui::Begin(DlgObj_SID[cast::to_szt(DlgObj::AboutDlg)], &opened,
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking))
    {
        if (!g::pTextures->get_image_alive(m_ico_id))
            m_ico_id = g::pTextures->load_image_from_path(m_ico_path, ABOUT_ICON_RECT, ABOUT_ICON_RECT, ABOUT_ICON_LIFETIME);

        std::optional<bool> ico_valid;
        ImTextureID image = nullptr;
        if (m_ico_id > 0)
            image = g::pTextures->get_image(m_ico_id, ico_valid);

        if (ImGui::BeginTable("##about_header_table", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, ABOUT_ICON_RECT + 50.0f);
            ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            if (image && ico_valid.value_or(false))
                ImGui::Image(image, ImVec2(ABOUT_ICON_RECT, ABOUT_ICON_RECT));
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(30, 0));

            ImGui::TableSetColumnIndex(1);
            ImGui::PushFont(g::pMain->get_bold_font());
            ImGui::TextUnformatted(APP_CAPTION);
            ImGui::PopFont();
            ImGuiEx::Spacing(2);

            if (ImGui::BeginTable("##info_table", 2, ImGuiTableFlags_None))
            {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Version:");
                ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(m_version.c_str());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Date:");
                ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(m_release_date.c_str());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Website:");
                ImGui::TableSetColumnIndex(1); 
                if (ImGuiEx::UrlButton(WEB_APP_HOME))
                    tools::path::open_file(WEB_APP_HOME, true);

                ImGui::TableNextRow();
                if (m_upd_found) ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeHeaderText));
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Update:");
                if (m_upd_found) ImGui::PopStyleColor();
                ImGui::TableSetColumnIndex(1); 

                if (m_upd_checked)
                {
                    if (!m_http_result)
                        ImGui::TextUnformatted("failed to check");
                    else if (!m_upd_found)
                        ImGui::TextUnformatted("no updates");
                    else
                    {
                        if (ImGuiEx::UrlButton(m_new_version.c_str(), "Download new version"))
                            tools::path::open_file(UPDATE_PATH, true);
                    }
                }
                else
                {
                    const bool checking = m_upd_checking;
                    if (checking) ImGui::BeginDisabled();
                    if (ImGui::Button(checking ? "Checking..." : "Check for updates") || m_autocheck)
                    {
                        m_autocheck = false;
                        m_upd_checking = true;
                        g::pFlow->check_for_updates(upd_callback);
                    }
                    if (checking) ImGui::EndDisabled();
                }

                ImGui::EndTable();
            }

            ImGui::EndTable();
        }

        ImGuiEx::Spacing(3);
        ImGui::Separator();
        ImGuiEx::Spacing(3);

        if (ImGui::BeginTabBar("##docs_tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("EULA"))
            {
                m_text_buffer = &m_license;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Privacy Policy"))
            {
                m_text_buffer = &m_privacy;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Third-party Notices"))
            {
                m_text_buffer = &m_third_party;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Readme"))
            {
                m_text_buffer = &m_readme;
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        if (nullptr == m_text_buffer)
            m_text_buffer = &m_license;

        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(20, 24, 30, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(50, 55, 65, 255));

        ImGuiEx::InputTextMultiline("##textblock", m_text_buffer, ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);

        ImGui::PopStyleColor(2);
    }

    return cleanup_and_exit(opened);
}

}
