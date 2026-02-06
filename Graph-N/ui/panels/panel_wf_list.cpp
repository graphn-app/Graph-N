
#include "ui/panels/panel_wf_list.h"
#include "core/workflow/workflow.h"
#include "core/managers/workflows_manager.h"
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

    setup_button_state_active();
    rebuild();
}

void PanelWfList::free()
{
    LogF;
    m_active = 0;
    m_wfs.clear();
    m_deleted.clear();
    m_labels.clear();
    m_panel_items.clear();
    m_panel_items_deleted.clear();
    m_panel_items_labels.clear();
    m_renaming_wf = -1;
}

void PanelWfList::setup_button_state_active()
{
    clear_toolbar();
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
        button.icon = ICON_LC_TRASH;
        button.tooltip = "Remove to Trash (Auto-purge on exit)";
        button.on_click = std::bind(&PanelWfList::on_delete_wf, this, std::placeholders::_1);
        button.on_preview = std::bind(&PanelWfList::on_pre_delete_wf, this, std::placeholders::_1);
        button.long_indent_after = true;
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_TAG;
        button.tooltip = "Manage labels";
        button.on_click = std::bind(&PanelWfList::on_manage_label, this, std::placeholders::_1);
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
}

void PanelWfList::setup_button_state_deleted()
{
    clear_toolbar();
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_TRASH;
        button.tooltip = "Close Trash";
        button.on_click = std::bind(&PanelWfList::on_view_deleted, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    finalize_toolbar();
}

void PanelWfList::setup_button_state_labels()
{
    clear_toolbar();
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_PLUS;
        button.tooltip = "Add new label";
        button.on_click = std::bind(&PanelWfList::on_add_label, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    finalize_toolbar();
}

void PanelWfList::stop_all_threads()
{
    LogF;
}

DlgCode PanelWfList::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        draw_toolbar();

        if (m_wfs.empty())
        {
            show_empty_panel_descr(
                "",
                {},
                {
                    "Your library of saved graphs.",
                    "Here you can create, rename, label,",
                    "and delete your Projects."
                    "",
                    "Deleted Projects will be",
                    "auto-purged on exit",
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

void PanelWfList::show_wfs()
{
    ImGuiEx::Spacing(3);
    if (ImGui::BeginChild("WfsGroup", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        PanelItemState state;
        if (m_deleted_view)
        {
            if (draw_list(m_panel_items_deleted, state))
                handle_dirty(state);
        }
        else
        {
            if (draw_list(m_panel_items, state))
                handle_dirty(state);
        }
    }
    ImGui::EndChild();
}

void PanelWfList::reload()
{
    const auto sort_callback = [](const WfInfo& left, const WfInfo& right)
    {
        return tools::str::to_lower_ascii(left.name) < tools::str::to_lower_ascii(right.name);
    };

    m_wfs = g::pWfsManager->get_wfs();
    std::sort(m_wfs.begin(), m_wfs.end(), sort_callback);
    m_deleted = g::pWfsManager->get_deleted();
    std::sort(m_deleted.begin(), m_deleted.end(), sort_callback);
    m_labels = g::pWfsManager->get_labels();
    rebuild();
}

void PanelWfList::rebuild()
{
    LogF;

    std::set<std::string> expanded;
    for (auto& item : m_panel_items)
    {
        if (item.expanded)
            expanded.insert(item.name);
    }

    m_panel_items.clear();
    m_panel_items_deleted.clear();
    m_panel_items_labels.clear();

    m_panel_items.reserve(m_wfs.size() + m_labels.size() + 1);
    m_panel_items_deleted.reserve(m_deleted.size());
    m_panel_items_labels.reserve(m_labels.size());

    m_active = g::pWfsManager->get_active();

    for (const auto& label : m_labels)
    {
        PanelListItem item;
        item.name = label;
        item.icon = ICON_LC_MINUS;
        item.icon_can_click = true;
        m_panel_items_labels.push_back(std::move(item));
    }
    if (m_new_label_state)
    {
        PanelListItem item;
        item.name = "New label";
        item.renaming = true;
        item.need_focus = true;
        m_panel_items_labels.push_back(std::move(item));
        m_new_label_state = false;
    }

    for (auto& wf : m_deleted)
    {
        PanelListItem item;
        item.name = wf.name;
        item.icon = ICON_LC_UNDO_2;
        item.icon_can_click = true;
        item.ptr = &wf;
        m_panel_items_deleted.push_back(std::move(item));
    }

    if (m_labels.empty())
    {
        for (auto& wf : m_wfs)
        {
            PanelListItem item;
            item.selected = (m_active == wf.id);
            if (item.selected)
            {
                item.icon = ICON_LC_CHECK_CHECK;
                item.icon_active = true;
            }
            item.name = wf.name;
            item.can_lclick = true;
            
            item.ptr = &wf;
            m_panel_items.push_back(std::move(item));
        }
    }
    else
    {
        {
            PanelListItem item_all;
            item_all.name = "All";
            item_all.can_expand = true;
            item_all.expanded = expanded.count("All") > 0;
            m_panel_items.push_back(std::move(item_all));
        }

        if (expanded.count("All") > 0)
        {
            for (auto& wf : m_wfs)
            {
                PanelListItem item;
                item.depth = 1;
                item.dbl_check_if_selected = true;
                item.selected = (wf.id == m_active);
                item.name = item.name_backup = wf.name;
                item.can_lclick = item.can_rclick = true;
                item.ptr = &wf;
                m_panel_items.push_back(std::move(item));
            }
        }

        for (const auto& label : m_labels)
        {
            {
                PanelListItem label_item;
                label_item.name = label;
                label_item.can_expand = true;
                label_item.expanded = expanded.count(label) > 0;
                m_panel_items.push_back(std::move(label_item));
            }

            if (expanded.count(label) == 0)
                continue;

            for (auto& wf : m_wfs)
            {
                if (g::pWfsManager->is_in_label(wf.id, label.c_str()))
                {
                    PanelListItem wf_item;
                    wf_item.depth = 1;
                    wf_item.dbl_check_if_selected = true;
                    wf_item.selected = (wf.id == m_active);
                    wf_item.name = wf_item.name_backup = wf.name;
                    wf_item.can_lclick = wf_item.can_rclick = true;
                    wf_item.ptr = &wf;
                    m_panel_items.push_back(std::move(wf_item));
                }
            }
        }
    }
}

void PanelWfList::handle_dirty(const PanelItemState& dirty_item)
{
    Log("Dirty. del-view=" << m_deleted_view << ",  idx=" << dirty_item.index << ", rc=" << dirty_item.rclicked << " lc=" << dirty_item.lclicked << ", ico=" << dirty_item.icon_clicked << ", xp=" << dirty_item.expanded);

    if (m_deleted_view && dirty_item.index >= m_panel_items_deleted.size() || !m_deleted_view && dirty_item.index >= m_panel_items.size())
    {
        LogError(ExcepCategory::Logic, "Index out of bounds");
    }

    if (dirty_item.expanded)
    {
        rebuild();
    }

    else if (m_deleted_view && dirty_item.icon_clicked)
    {
        auto& item = m_panel_items_deleted[dirty_item.index];
        auto* info = ((WfInfo*)item.ptr);
        g::pFlow->wf_restore(info->id);
    }

    else if (m_labels_view && dirty_item.icon_clicked)
    {
        auto& item = m_panel_items_deleted[dirty_item.index];
        g::pFlow->wf_remove_label(item.name);
    }

    else if (!m_deleted_view && !m_labels_view)
    {
        auto& item = m_panel_items[dirty_item.index];
        auto* info = ((WfInfo*)item.ptr);

        if (dirty_item.lclicked)
        {
            g::pFlow->wf_activate(info->id);
        }
        else if (dirty_item.rclicked)
        {
            // TODO
        }
        else if (dirty_item.renamed)
        {
            g::pFlow->wf_rename(info->id, item.name);
        }
    }
}

void PanelWfList::on_add_new_wf(PanelToolbar::Button& /*button*/)
{
    g::pFlow->wf_create();
}

void PanelWfList::on_rename_wf(PanelToolbar::Button& /*button*/)
{
    m_renaming_wf = cast::to_i64(m_active);
    rebuild();
}

void PanelWfList::on_pre_rename_wf(PanelToolbar::Button& button)
{
    button.enabled = (m_active != 0);
}

void PanelWfList::on_locate_wf(PanelToolbar::Button& /*button*/)
{
    g::pFlow->wf_locate(m_active);
}

void PanelWfList::on_pre_locate_wf(PanelToolbar::Button& button)
{
    button.enabled = (m_active != 0);
}

void PanelWfList::on_delete_wf(PanelToolbar::Button& /*button*/)
{
    g::pFlow->wf_delete(m_active);
}

void PanelWfList::on_pre_delete_wf(PanelToolbar::Button& button)
{
    button.enabled = (m_active != 0);
}

void PanelWfList::on_view_deleted(PanelToolbar::Button& /*button*/)
{
    m_deleted_view = !m_deleted_view;
    if (m_deleted_view)
        setup_button_state_deleted();
    else
        setup_button_state_active();
}

void PanelWfList::on_manage_label(PanelToolbar::Button& /*button*/)
{
    m_labels_view = !m_labels_view;
    if (m_deleted_view)
        setup_button_state_labels();
    else
        setup_button_state_active();
}

void PanelWfList::on_add_label(PanelToolbar::Button& /*button*/)
{
    m_new_label_state = true;
    rebuild();
}

} // namespace ui
