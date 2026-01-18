
#include "ui/panels/panel_wf_list.h"
#include "core/workflow/workflow.h"
#include "core/managers/settings_manager.h"
#include "core/managers/db_manager.h"
#include "ui/graph-n_flow.h"

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

    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_PLUS;
        button.tooltip = "New Project";
        button.on_click = std::bind(&PanelWfList::on_add_new_wf, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_PEN_LINE;
        button.tooltip = "Rename Project";
        button.on_click = std::bind(&PanelWfList::on_rename_wf, this, std::placeholders::_1);
        button.on_preview = std::bind(&PanelWfList::on_pre_rename_wf, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_SCAN_EYE;
        button.tooltip = "Locate active workflow";
        button.on_click = std::bind(&PanelWfList::on_locate_wf, this, std::placeholders::_1);
        button.on_preview = std::bind(&PanelWfList::on_pre_locate_wf, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_ARROW_DOWN_A_Z;
        button.tooltip = "Sort by Name/Date";
        button.on_click = std::bind(&PanelWfList::on_sort_wfs, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_TRASH;
        button.tooltip = "Remove to Trash (Auto-purge on exit)";
        button.on_click = std::bind(&PanelWfList::on_delete_wf, this, std::placeholders::_1);
        button.on_preview = std::bind(&PanelWfList::on_pre_delete_wf, this, std::placeholders::_1);
        button.long_indent_after = true;
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_TRASH_2;
        button.tooltip = "Trash.\nView deleted Projects";
        button.on_click = std::bind(&PanelWfList::on_view_deleted, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    finalize_toolbar();

    rebuild_items();
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
        draw_toolbar();

        if (m_wfs.empty())
        {
            show_empty_panel_descr(
                "",
                {},
                {
                    "Your library of saved graphs.",
                    "Here you can create, rename,",
                    "open and delete your Projects.",
                    "",
                    "Deleted Projects will be auto-purged on exit",
                    "",
                    "Use the Main Menu to",
                    "import or export .json files."
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
    if (nullptr == wf)
    {
        rebuild_items();
        return;
    }

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
    ImGuiEx::Spacing(3);
    if (ImGui::BeginChild("WfsGroup", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        PanelItemState state;
        if (draw_list(m_panel_items, state))
            handle_dirty(state);
    }
    ImGui::EndChild();
}

void PanelWfList::rebuild_items()
{
    LogF;

    m_panel_items.clear();
    m_panel_items.reserve(m_wfs.size() + 1);

    for (auto wf : m_wfs)
    {
        auto& data = wf->get_workflow_data();
        if (data.fav)
        {
            PanelListItem item;
            item.name = data.name;
            item.can_star = item.starred = true;
            item.can_lclick = true;
            item.selected = (wf->get_id() == m_active->get_id());
            item.ptr = wf;
            m_panel_items.push_back(std::move(item));
        }
    }

    if (!m_panel_items.empty())
    {
        PanelListItem separator;
        separator.is_separator = true;
        m_panel_items.push_back(std::move(separator));
    }

    for (auto wf : m_wfs)
    {
        auto& data = wf->get_workflow_data();
        if (!data.fav)
        {
            PanelListItem item;
            item.name = data.name;
            item.can_star = true;
            item.can_lclick = true;
            item.selected = (wf->get_id() == m_active->get_id());
            item.ptr = wf;
            m_panel_items.push_back(std::move(item));
        }
    }
}

void PanelWfList::handle_dirty(const PanelItemState& dirty_item)
{
    Log("Dirty: idx=" << dirty_item.index << ", rc=" << dirty_item.rclicked << " lc=" << dirty_item.lclicked << ", fv=" << dirty_item.starred << ", xp=" << dirty_item.expanded);

    if (dirty_item.index >= m_panel_items.size())
    {
        LogError(ExcepCategory::Logic, "Index out of bounds");
    }

    auto& panel_item = m_panel_items[dirty_item.index];
    data::Workflow* wf = (data::Workflow*)panel_item.ptr;
    if (nullptr == wf)
    {
        LogError(ExcepCategory::Logic, "Null dirty item");
    }

    if (dirty_item.starred)
    {
        auto& data = wf->get_workflow_data();
        data.fav = !data.fav;
        g::pDbManager->wf_update_meta(data);
        rebuild_items();
    }
    else if (dirty_item.lclicked)
    {
        g::pFlow->set_active_wf(wf);
    }
    else
    {
        LogError(ExcepCategory::Logic, "Unhandled dirty state");
    }
}

void PanelWfList::on_add_new_wf(PanelToolbar::Button& /*button*/)
{
    g::pFlow->add_new_workflow();
    rebuild_items();
}

void PanelWfList::on_rename_wf(PanelToolbar::Button& /*button*/)
{}

void PanelWfList::on_pre_rename_wf(PanelToolbar::Button& button)
{
    button.enabled = (m_active != nullptr);
}

void PanelWfList::on_locate_wf(PanelToolbar::Button& /*button*/)
{}

void PanelWfList::on_pre_locate_wf(PanelToolbar::Button& button)
{
    button.enabled = (m_active != nullptr);
}

void PanelWfList::on_sort_wfs(PanelToolbar::Button& /*button*/)
{}

void PanelWfList::on_delete_wf(PanelToolbar::Button& /*button*/)
{}

void PanelWfList::on_pre_delete_wf(PanelToolbar::Button& button)
{
    button.enabled = (m_active != nullptr);
}

void PanelWfList::on_view_deleted(PanelToolbar::Button& /*button*/)
{}

} // namespace ui
