#include "ui/panels/panel_wf_list.h"
#include "core/workflow/workflow.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "ui/graph-n_flow.h"

namespace ui
{

PanelWfList::PanelWfList()
    : PanelBase(DlgObj::PanelWfList, "Projects")
{}

PanelWfList::~PanelWfList() noexcept = default;

void PanelWfList::init()
{
    PanelBase::init();

    m_tt_callbacks.on_show_tt = [this](int32_t idx) noexcept { on_show_tt(idx); };

    create_toolbar();
    rebuild_items();

    g::pHotkeysManager->add_listener(this);
}

void PanelWfList::stop_and_cleanup() noexcept
{
    m_active = 0;
    m_wfs.clear(); m_wfs.shrink_to_fit();
    m_panel_items.clear(); m_panel_items.shrink_to_fit();
    m_new_label = 0;
}

void PanelWfList::handle_hotkeys_changed()
{
    create_toolbar();
}

void PanelWfList::create_toolbar()
{
    clear_toolbar();
    if (!m_deleted_mode && !m_labels_mode)
    {
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_PLUS;
            el.tooltip = "New Project";
            el.tooltip_hotkey = g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Project_CreateNew);
            el.on_click = [this](PanelToolbar::Element& el) { on_add_new_wf(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_PEN_LINE;
            el.tooltip = "Rename Project";
            el.tooltip_hotkey = g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Project_Rename);
            el.on_click = [this](PanelToolbar::Element& el) noexcept { on_rename_wf(el); };
            el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_rename_wf(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_COPY;
            el.tooltip = "Duplicate Project";
            el.on_click = [this](PanelToolbar::Element& el) { on_copy_wf(el); };
            el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_copy_wf(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_TRASH;
            el.tooltip = "Remove to Trash";
            el.on_click = [this](PanelToolbar::Element& el) { on_delete_wf(el); };
            el.on_preview = [this](PanelToolbar::Element& el) noexcept { on_pre_delete_wf(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_TAGS;
            el.tooltip = "Manage labels";
            el.on_click = [this](PanelToolbar::Element& el) { on_labels_mode(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_TRASH_2;
            el.tooltip = "Trash (Auto-purge on exit)\nView deleted Projects";
            el.on_click = [this](PanelToolbar::Element& el) { on_deleted_mode(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }
    }
    else if (m_deleted_mode)
    {
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_MOVE_LEFT;
            el.tooltip = "Back to Projects";
            el.on_click = [this](PanelToolbar::Element& el) { on_deleted_mode(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }
    }
    else if (m_labels_mode)
    {
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_MOVE_LEFT;
            el.tooltip = "Back to Projects";
            el.on_click = [this](PanelToolbar::Element& el) { on_labels_mode(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_PLUS;
            el.tooltip = "Add new label";
            el.on_click = [this](PanelToolbar::Element& el) { on_add_label(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }
    }
    finalize_toolbar(true /*separator*/);
}

DlgCode PanelWfList::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        if (g::pFlow->get_nodes_ready()) [[likely]]
            draw_toolbar();

        if (!m_labels_mode && !m_deleted_mode && m_wfs.empty())
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
        else if (m_labels_mode && g::pWfsManager->get_labels().empty())
        {
            show_empty_panel_descr(
                "",
                {
                    ICON_LC_TAGS,
                    "Organize your Projects using labels.",
                    "A single Project can belong",
                    "to several labels at once.",
                    "",
                    "Use the context menu, double-click",
                    "or press Del, F2",
                    "to manage them.",
                },
                {
                    "To get started,",
                },
                "click here",
                [this]()
                {
                    m_new_label = g::pWfsManager->create_label();
                    rebuild_items();
                }
            );
        }
        else if (m_deleted_mode && m_wfs.empty())
        {
            show_empty_panel_descr(
                "",
                {
                    ICON_LC_TRASH_2,
                    "Your deleted Projects are kept here",
                    "You can restore them,",
                    "otherwise they will be purged",
                    "upon the next app restart.",
                },
                {}
            );
        }
        else if (!g::pFlow->get_nodes_ready())
        {
            show_empty_panel_descr(
                ICON_LC_INFO " Create, manage, and organize your projects.",
                {
                    ICON_LC_UNPLUG,
                    "Waiting for connection.",
                    "Projects are unavailable."
                },
                {}
            );
        }
        else [[likely]]
        {
            show_wfs();
        }
    }
    return end_panel(opened);
}

void PanelWfList::show_wfs()
{
    ImGuiEx::Spacing(3);
    if (ImGui::BeginChild(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_ItemsList)], ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        PanelItemEvent event;
        if (draw_list(m_panel_items, event, (m_deleted_mode ? &m_tt_callbacks : nullptr)))
            handle_dirty(event);
        draw_popups();
    }
    ImGui::EndChild();
}

void PanelWfList::rebuild_items()
{
    if (m_labels_mode)
    {
        rebuild_labels();
        return;
    }

    const auto sort_callback = [](const mngr::WfData* left, const mngr::WfData* right)
    {
        return tools::str::to_lower_ascii(left->name) < tools::str::to_lower_ascii(right->name);
    };

    if (m_deleted_mode)
    {
        m_wfs = g::pWfsManager->get_deleted();
        std::ranges::sort(m_wfs, sort_callback);
        rebuild_trash();
        return;
    }

    m_wfs = g::pWfsManager->get_undeleted();
    std::ranges::sort(m_wfs, sort_callback);

    std::set<wid> all_opened_wfs;
    m_active = g::pFlow->wf_get_active(&all_opened_wfs);

    const auto get_wf_name = [&wfs = m_wfs](wid id)
    {
        for (auto& wf : wfs)
        {
            if (id == wf->id)
                return wf->name;
        }
        return std::string("");
    };

    std::set<uint32_t> expanded;
    for (auto& item : m_panel_items)
    {
        if (item.expanded)
            expanded.insert(item.ext_data.label);
    }

    auto& labels = g::pWfsManager->get_labels();

    std::ranges::for_each(labels, [this, &expanded](const mngr::Label& l) { if (!m_curr_labels.contains(l.id)) expanded.insert(l.id); });
    m_curr_labels.clear();
    std::ranges::for_each(labels, [this](const mngr::Label& l) { m_curr_labels.insert(l.id); });

    m_panel_items.clear();
    m_panel_items.reserve(m_wfs.size() * 2);
    m_active = g::pWfsManager->get_active();

    if (!labels.empty())
    {
        PanelListItem label_all;
        label_all.name = "All";
        label_all.name_label = std::to_string(m_wfs.size());
        label_all.can_expand = true;
        label_all.expanded = expanded.contains(0);
        bool label_expanded = label_all.expanded;
        label_all.icon = label_expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER;
        label_all.ext_data.label = 0;
        m_panel_items.push_back(std::move(label_all));

        if (label_expanded)
        {
            for (const auto* wf_data : m_wfs)
            {
                PanelListItem item;
                item.depth = 1;
                item.name = wf_data->name;
                if (wf_data->id == m_active)
                {
                    item.icon = ICON_LC_CHECK_CHECK;
                    item.icon_always = true;
                }
                else
                    item.icon = ICON_LC_TAG;
                item.icon_tooltip = "Manage Project labels";
                item.icon_can_click = true;
                item.selected = all_opened_wfs.contains(wf_data->id);
                item.ext_data.ptr = cast::to_void(wf_data->id);
                m_panel_items.push_back(std::move(item));
            }
        }

        for (auto& label : labels)
        {
            const auto wfs_in_label = g::pWfsManager->get_wfs_in_label(label.id);

            PanelListItem label_item;
            label_item.name = label.name;
            label_item.name_label = std::to_string(wfs_in_label.size());
            label_item.can_expand = true;
            label_item.expanded = expanded.contains(label.id);
            label_expanded = label_item.expanded;
            label_item.icon = label_item.expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER;
            label_item.ext_data.label = label.id;
            m_panel_items.push_back(std::move(label_item));

            if (label_expanded)
            {
                for (auto wf_id : wfs_in_label)
                {
                    PanelListItem item;
                    item.depth = 1;
                    item.name = get_wf_name(wf_id);
                    if (wf_id == m_active)
                    {
                        item.icon = ICON_LC_CHECK_CHECK;
                        item.icon_always = true;
                    }
                    else
                        item.icon = ICON_LC_TAG;
                    item.icon_tooltip = "Manage Project labels";
                    item.icon_can_click = true;
                    item.selected = all_opened_wfs.contains(wf_id);
                    item.ext_data.ptr = cast::to_void(wf_id);
                    m_panel_items.push_back(std::move(item));
                }
            }
        }
    }
    else
    {
        for (auto wf_data : m_wfs)
        {
            PanelListItem item;
            item.name = wf_data->name;
            if (wf_data->id == m_active)
            {
                item.icon = ICON_LC_CHECK_CHECK;
                item.icon_always = true;
            }
            else
                item.icon = ICON_LC_TAG;
            item.icon_tooltip = "Manage Project labels";
            item.icon_can_click = true;
            item.selected = all_opened_wfs.contains(wf_data->id);
            item.ext_data.ptr = cast::to_void(wf_data->id);
            m_panel_items.push_back(std::move(item));
        }
    }
}

void PanelWfList::handle_dirty(const PanelItemEvent& event)
{
    LogRAssert(event.index < m_panel_items.size(), "Index out of bounds", );

    if (event.expand_toggled)
    {
        m_panel_items[event.index].expanded = !m_panel_items[event.index].expanded;
        rebuild_items();
        return;
    }

    if (m_labels_mode)
    {
        handle_dirty_labels(event);
        return;
    }

    if (m_deleted_mode)
    {
        handle_dirty_trash(event);
        return;
    }

    auto& item = m_panel_items[event.index];
    const wid id = cast::from_ptr<wid>(item.ext_data.ptr);

    if (event.icon_clicked && event.l_released)
    {
        m_active_popup_ptr = item.ext_data.ptr; 
        m_active_popup_labels.clear();
        *m_new_label_buff = 0;

        auto& labels = g::pWfsManager->get_labels();
        for (auto& label : labels)
        {
            if (g::pWfsManager->get_is_wf_in_label(id, label.id))
                m_active_popup_labels.insert(label.id);
        }

        ImGui::OpenPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_ItemInLabels)]);
        return;
    }

    m_active_wf_for_menu = 0;
    if (event.l_clicked && !event.icon_clicked)
    {
        g::pFlow->wf_activate(id);
    }
    else if (event.rename_completed)
    {
        g::pFlow->wf_rename(id, event.new_name);
        g::pFlow->host_handle_renaming(id, 0);
    }
    else if (event.r_released && !event.icon_clicked)
    {
        m_active_wf_for_menu = id;
        ImGui::OpenPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_ItemsMenu)]);
    }
}

void PanelWfList::rebuild_labels()
{
    auto& labels = g::pWfsManager->get_labels();

    m_panel_items.clear();
    m_panel_items.reserve(labels.size());

    for (auto& label : labels)
    {
        PanelListItem item;
        item.name = label.name;
        item.ext_data.label = label.id;

        if (m_new_label == label.id)
        {
            begin_rename(cast::to_i32(m_panel_items.size()), item.name);
            m_new_label = 0;
        }
        m_panel_items.push_back(std::move(item));
    }
}

void PanelWfList::handle_dirty_labels(const PanelItemEvent& event)
{
    auto& panel_item = m_panel_items[event.index];

    if (event.rename_completed)
    {
        if (g::pWfsManager->rename_label(panel_item.ext_data.label, event.new_name))
            panel_item.name = event.new_name;
        rebuild_items();
    }
    else if (event.r_released)
    {
        m_active_popup_label_id = panel_item.ext_data.label;
        ImGui::OpenPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_LabelMenu)]);
    }
    else if (event.l_released)
    {
        begin_rename(event.index, panel_item.name);
    }
}

void PanelWfList::rebuild_trash()
{
    m_panel_items.clear();
    m_panel_items.reserve(m_wfs.size());

    for (auto wf_data : m_wfs)
    {
        PanelListItem item;
        item.name = wf_data->name;
        item.icon = ICON_LC_UNDO_2;
        item.icon_tooltip = "Undelete Project";
        item.ext_data.ptr = cast::to_void(wf_data->id);
        m_panel_items.push_back(std::move(item));
    }
}

void PanelWfList::handle_dirty_trash(const PanelItemEvent& event)
{
    if ((event.icon_clicked || event.l_clicked) && !event.l_released)
    {
        const wid id = cast::from_ptr<wid>(m_panel_items[event.index].ext_data.ptr);
        g::pFlow->wf_restore(id);
    }
}

void PanelWfList::draw_popups()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(300, 400));
    if (m_active_wf_for_menu != 0 && ImGui::BeginPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_ItemsMenu)], ImGuiWindowFlags_NoMove))
    {
        auto wf = g::pWfsManager->access(m_active_wf_for_menu, true);
        if (nullptr == wf)
        {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }

        bool close = false;
        if (ImGui::MenuItem("Rename project"))
        {
            rename_project(m_active_wf_for_menu);
            close = true;
        }
        if (ImGui::MenuItem("Duplicate project"))
        {
            g::pFlow->wf_create_copy_from(m_active_wf_for_menu, std::nullopt, std::nullopt);
            close = true;
        }
        if (ImGui::MenuItem("Remove project"))
        { 
            g::pFlow->wf_delete(m_active_wf_for_menu);
            close = true;
        }
        if (wf->get_id() == m_active && ImGui::MenuItem("Close other Projects"))
        {
            g::pFlow->on_menu_close_all_projects(true);
        }

        if (close)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    else if (m_labels_mode && ImGui::BeginPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_LabelMenu)], ImGuiWindowFlags_NoMove))
    {
        bool close = false;
        const uint32_t current_label_id = m_active_popup_label_id;

        if (ImGui::MenuItem("Add new label"))
        {
            m_new_label = g::pWfsManager->create_label();
            close = true;
            rebuild_items();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Rename label"))
        {
            for (int i = 0; i < m_panel_items.size(); i++)
            {
                if (m_panel_items[i].ext_data.label == current_label_id)
                {
                    begin_rename(i, m_panel_items[i].name);
                    break;
                }
            }
            close = true;
        }
        if (ImGui::MenuItem("Delete label"))
        {
            g::pWfsManager->remove_label(current_label_id);
            close = true;
            rebuild_items();
        }

        if (close)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    else if (!m_labels_mode && ImGui::BeginPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_ItemInLabels)], ImGuiWindowFlags_NoMove))
    {
        ImGui::TextDisabled("Assign label(s):");
        ImGui::Separator();

        bool changed = false;
        auto& labels = g::pWfsManager->get_labels();
        const wid active_wf = cast::from_ptr<wid>(m_active_popup_ptr);

        for (auto& label : labels)
        {
            bool in_label = m_active_popup_labels.contains(label.id);
            ImGui::PushID(label.id);
            if (ImGui::Checkbox(label.name.c_str(), &in_label))
            {
                changed = true;
                if (in_label) m_active_popup_labels.insert(label.id);
                else          m_active_popup_labels.erase(label.id);

                g::pWfsManager->set_wf_labels(active_wf, m_active_popup_labels);
                rebuild_items();
            }
            ImGui::PopID();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("New label:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##newlabel", m_new_label_buff, 30, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit())
        {
            if (*m_new_label_buff != 0 && g::pWfsManager->find_label(m_new_label_buff) == 0 && tools::str::to_lower_ascii(m_new_label_buff) != "all")
            {
                uint32_t new_id = g::pWfsManager->create_label(m_new_label_buff);
                m_active_popup_labels.insert(new_id);
                g::pWfsManager->set_wf_labels(active_wf, m_active_popup_labels);
                *m_new_label_buff = 0;
                rebuild_items();
            }
        }
        ImGui::EndPopup();
    }
}

void PanelWfList::on_show_tt(int32_t /*idx*/) noexcept
{
    if (m_deleted_mode)
    {
        ImGui::TextUnformatted("Undelete Project");
    }
}

void PanelWfList::on_add_new_wf(PanelToolbar::Element& /*el*/)
{
    g::pFlow->wf_create();
}

void PanelWfList::rename_project(wid wf_id) noexcept
{
    if (!g::pFlow->get_nodes_ready())
        return;

    for (int32_t i = 0; i < cast::to_i32(m_panel_items.size()); ++i)
    {
        if (cast::from_ptr<wid>(m_panel_items[i].ext_data.ptr) == wf_id)
        {
            // Trigger the inline renaming mode in the base class
            begin_rename(i, m_panel_items[i].name);
            break;
        }
    }
}

void PanelWfList::on_rename_wf(PanelToolbar::Element& /*el*/) noexcept
{
    rename_project(m_active);
}

void PanelWfList::on_pre_rename_wf(PanelToolbar::Element& el) noexcept
{
    el.enabled = (m_active != 0);
}

void PanelWfList::on_copy_wf(PanelToolbar::Element& /*el*/)
{
    g::pFlow->wf_create_copy_from(m_active, std::nullopt, std::nullopt);
}

void PanelWfList::on_pre_copy_wf(PanelToolbar::Element& el) noexcept
{
    el.enabled = (m_active != 0);
}

void PanelWfList::on_delete_wf(PanelToolbar::Element& /*el*/)
{
    g::pFlow->wf_delete(m_active);
}

void PanelWfList::on_pre_delete_wf(PanelToolbar::Element& el) noexcept
{
    el.enabled = (m_active != 0);
}

void PanelWfList::on_deleted_mode(PanelToolbar::Element& /*el*/)
{
    m_deleted_mode = !m_deleted_mode;
    create_toolbar();
    rebuild_items();
    reset_toolbar_state();
}

void PanelWfList::on_labels_mode(PanelToolbar::Element& /*el*/)
{
    m_labels_mode = !m_labels_mode;
    create_toolbar();
    rebuild_items();
    reset_toolbar_state();
}

void PanelWfList::on_add_label(PanelToolbar::Element& /*el*/)
{
    m_new_label = g::pWfsManager->create_label();
    rebuild_items();
}

} // namespace ui
