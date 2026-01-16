
#include "ui/panels/panel_wf_list.h"
#include "core/workflow/workflow.h"
#include "core/managers/settings_manager.h"
#include "platform/main.h"
#include "ui/font_guard.h"

namespace ui
{

PanelWfList::PanelWfList()
    : PanelBase(DlgObj::PanelWfList, ICON_LC_LIBRARY " Projects", ICON_LC_LIBRARY, "0EE80880-D72D-48A6-808A-187FE7A17A71")
{}

PanelWfList::~PanelWfList()
{
    LogF;
}

void PanelWfList::init()
{
    LogF;
    PanelBase::init();

    const auto adjust_brightness = [](ImU32 color, float factor)
    {
        ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);

        c.x = std::min(c.x * factor, 1.0f); // R
        c.y = std::min(c.y * factor, 1.0f); // G
        c.z = std::min(c.z * factor, 1.0f); // B

        return ImGui::ColorConvertFloat4ToU32(c);
    };

    data()->clr_hover_btn = adjust_brightness(data()->clr_bg, 1.5f);
    data()->clr_active_btn = adjust_brightness(data()->clr_bg, 1.7f);
    data()->clr_active = g::pOptsManager->color_load(AppColor::SelectedItem);
}

void PanelWfList::stop_all_threads()
{
    LogF;
}

DlgCode PanelWfList::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
    {
        if (m_wfs.empty())
        {
            show_empty_panel_descr(
                ICON_LC_INFO " My Projects.",
                {},
                {
                    "Your library of saved graphs.",
                    "Here you can create, rename,"
                    "open and delete your Projects.",
                    "",
                    "Deleted Projects will be auto-purged on exit",
                    "",
                    "Use the Main Menu to import or export .json files."
                }
                );
        }
        else
            show_wfs();
    }
    return end_panel(opened);
}

void PanelWfList::add_wf(data::Workflow* wf)
{
    for (auto saved : m_wfs)
    {
        if (wf == saved)
        {
            LogError(ExcepCategory::Logic, "Wf added twice");
            return;
        }
    }
    m_wfs.push_back(wf);
}

void PanelWfList::remove_wf(data::Workflow* wf)
{
    m_wfs.remove_if([wf](data::Workflow* pwf) { return wf->get_id() == pwf->get_id(); });
}

void PanelWfList::set_active(data::Workflow* wf)
{
    m_active_updated = (m_active != wf);
    m_active = wf;
    g::pOptsManager->wf_active_save(m_active->get_id());
}

void PanelWfList::show_wfs()
{
    show_cfg_buttons();
    ImGuiEx::Spacing(3);
    if (ImGui::BeginChild("WfsGroup", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        show_wfs_list();
    }
    ImGui::EndChild();
}

void PanelWfList::show_cfg_buttons()
{
    ImGui::PushStyleColor(ImGuiCol_Button, data()->clr_bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, data()->clr_hover_btn);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, data()->clr_active_btn);

    // import
    // import + export -> main menu?
    // ICON_LC_STAR

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const uint32_t button_count = 8;
    const uint32_t big_spaces = 2;
    const uint32_t small_spaces = 1;
    const float start_pos =
        ImGui::GetContentRegionAvail().x -
        (button_count * data()->button_width) -
        (big_spaces * spacing * data()->ungrouped_btn_spacing_x) -
        (small_spaces * spacing * data()->grouped_btn_spacing_x) -
        spacing * 3;

    ImGui::SameLine(start_pos);

    FontGuard fg(g::pMain->get_icons_font());

    const auto tooltip = [&fg](const char* text)
    {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        {
            fg.pop();
            if (ImGui::BeginTooltip())
            {
                ImGui::TextUnformatted(text);
                ImGui::EndTooltip();
            }
        }
    };

    if (ImGui::Button(ICON_LC_PLUS))
    {
        add_new_wf();
    }
    tooltip("New Project");

    ImGui::SameLine(0.0f, spacing * data()->ungrouped_btn_spacing_x);
    fg.push();
    if (ImGui::Button(ICON_LC_PEN_LINE))
    {
        rename_wf();
    }
    tooltip("Rename Project");

    ImGui::SameLine(0.0f, spacing * data()->grouped_btn_spacing_x);
    fg.push();
    if (ImGui::Button(ICON_LC_SCAN_EYE))
    {
        locate_wf();
    }
    tooltip("Locate active workflow");

    ImGui::SameLine(0.0f, spacing * data()->ungrouped_btn_spacing_x);
    fg.push();
    if (ImGui::Button(ICON_LC_SHARE))
    {
        export_wf();
    }
    tooltip("Export to JSON");

    ImGui::SameLine(0.0f, spacing * data()->ungrouped_btn_spacing_x);
    fg.push();
    if (ImGui::Button(ICON_LC_ARROW_DOWN_A_Z)) // ICON_LC_CLOCK_ARROW_DOWN
    {
        sort_wfs();
    }
    tooltip("Sort by Name/Date");

    ImGui::SameLine(0.0f, spacing * data()->ungrouped_btn_spacing_x);
    fg.push();
    if (ImGui::Button(ICON_LC_TRASH))
    {
        delete_wf();
    }
    tooltip("Remove to Trash (Auto-purge on exit)");

    ImGui::SameLine(0.0f, spacing * data()->ungrouped_btn_spacing_x);
    fg.push();
    if (ImGui::Button(ICON_LC_TRASH_2))
    {
        view_deleted();
    }
    tooltip("Trash.\nView deleted Projects");
    // Deleted Projects. Will be purged automatically on exit
    // Projects in this list are marked for deletion. They will be permanently removed from the database when you close the application

    ImGui::PopStyleColor(3);
}

void PanelWfList::show_wfs_list()
{}

} // namespace ui
