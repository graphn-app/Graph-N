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

void PanelWfList::free() noexcept
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
        PanelItemState state;
        if (draw_list(m_panel_items, state, (m_deleted_mode ? &m_tt_callbacks : nullptr)))
            handle_dirty(state);
    }
    ImGui::EndChild();
}

void PanelWfList::rebuild_items()
{
    const auto sort_callback = [](const mngr::WfData* left, const mngr::WfData* right)
    {
        return tools::str::to_lower_ascii(left->name) < tools::str::to_lower_ascii(right->name);
    };

    if (m_labels_mode)
    {
        rebuild_labels();
        return;
    }

    if (m_deleted_mode)
    {
        m_wfs = g::pWfsManager->get_deleted();
        std::ranges::sort(m_wfs, sort_callback);

        rebuild_trash();
        return;
    }

    m_wfs = g::pWfsManager->get_undeleted();
    std::ranges::sort(m_wfs, sort_callback);

    struct WfLabelState final : public PanelListItem::State
    {
        std::set<uint32_t> active_labels;
    };

    static char new_label_buff[31] = {};

    const auto on_icon_handler = [&labels_mode = m_labels_mode]
        (PanelListItem& item, bool /*l_clicked*/, bool /*l_dbl_clicked*/, bool l_released, bool /*r_clicked*/, bool r_released, bool& handler_processed)
    {        
        bool dirty = false;
        auto& labels = g::pWfsManager->get_labels();
        if (!item.continue_processing_ico_handler && (l_released || r_released))
        {
            //if (labels.empty())
            //{
            //    item.continue_processing_ico_handler = false;
            //    labels_mode = true;
            //    return true;
            //}

            handler_processed = true;
            item.item_state = std::make_unique<WfLabelState>();
            item.item_state->ptr = item.ptr;
            WfLabelState* state = ((WfLabelState*)item.item_state.get());
            for (auto& label : labels)
            {
                if (g::pWfsManager->get_is_wf_in_label(cast::from_ptr<wid>(item.ptr), label.id))
                {
                    state->active_labels.insert(label.id);
                }
            }

            ImGui::OpenPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_LabelsMenu)]);
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(ImGui::GetContentRegionAvail().x, 400));
        if (ImGui::BeginPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_LabelsMenu)]))
        {
            handler_processed = true;
            item.continue_processing_ico_handler = true;

            ImGui::TextDisabled("Assign label(s):");
            ImGui::Separator();

            for (auto& label : labels)
            {
                WfLabelState* state = ((WfLabelState*)item.item_state.get());
                bool wf_in_label = state->active_labels.contains(label.id);

                ImGui::PushID(label.id);
                if (ImGui::Checkbox(label.name.c_str(), &wf_in_label))
                {
                    item.item_state->changed = true;
                    if (wf_in_label)
                        state->active_labels.insert(label.id);
                    else
                        state->active_labels.erase(label.id);
                }
                ImGui::PopID();
            }

            if (!item.item_state->changed)
                ImGui::BeginDisabled();
            if (ImGui::Button("Apply", ImVec2(-FLT_MIN, 0)))
            {
                item.continue_processing_ico_handler = false;
                ImGui::CloseCurrentPopup();
                *new_label_buff = 0;

                if (item.item_state->changed && item.item_state->ptr == item.ptr)
                {
                    dirty = true;
                    g::pWfsManager->set_wf_labels(cast::from_ptr<wid>(item.ptr), ((WfLabelState*)item.item_state.get())->active_labels);            
                }
            }
            if (!item.item_state->changed)
                ImGui::EndDisabled();

            ImGui::Spacing();
            ImGui::TextDisabled("New label:");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##newlabel", new_label_buff, 30, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit())
            {
                if (*new_label_buff != 0 && g::pWfsManager->find_label(new_label_buff) == 0)
                {
                    g::pWfsManager->create_label(new_label_buff);
                }
            }


            ImGui::EndPopup();
        }
        else
            item.continue_processing_ico_handler = false;

        if (!item.continue_processing_ico_handler)
            item.item_state.reset();
        return dirty;
    };

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
            expanded.insert(item.label);
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
        label_all.name_p2 = std::to_string(m_wfs.size());
        label_all.can_expand = true;
        label_all.expanded = expanded.contains(0);
        bool label_expanded = label_all.expanded;
        label_all.icon = label_expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER;
        label_all.label = 0;
        m_panel_items.push_back(std::move(label_all));

        if (label_expanded)
        {
            for (auto wf_data : m_wfs)
            {
                PanelListItem item;
                item.depth = 1;
                item.name = wf_data->name;
                item.icon = ICON_LC_TAG;
                item.icon_tooltip = "Manage Project labels";
                item.icon_can_click = true;
                item.can_lclick = true;
                item.selected = (wf_data->id == m_active);
                item.on_icon_handler = on_icon_handler;
                item.ptr = cast::to_void(wf_data->id);
                m_panel_items.push_back(std::move(item));
            }
        }

        for (auto& label : labels)
        {
            const auto wfs_in_label = g::pWfsManager->get_wfs_in_label(label.id);

            PanelListItem label_item;
            label_item.name = label.name;
            label_item.name_p2 = std::to_string(wfs_in_label.size());
            label_item.can_expand = true;
            label_item.expanded = expanded.contains(label.id);
            label_expanded = label_item.expanded;
            label_item.icon = label_item.expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER;
            label_item.label = label.id;
            m_panel_items.push_back(std::move(label_item));

            if (label_expanded)
            {
                for (auto wf_id : wfs_in_label)
                {
                    PanelListItem item;
                    item.depth = 1;
                    item.name = get_wf_name(wf_id);
                    item.icon = ICON_LC_TAG;
                    item.icon_tooltip = "Manage Project labels";
                    item.icon_can_click = true;
                    item.can_lclick = true;
                    item.selected = (wf_id == m_active);
                    item.on_icon_handler = on_icon_handler;
                    item.ptr = cast::to_void(wf_id);
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
            item.icon = ICON_LC_TAG;
            item.icon_tooltip = "Manage Project labels";
            item.icon_can_click = true;
            item.can_lclick = true;
            item.selected = (wf_data->id == m_active);
            item.on_icon_handler = on_icon_handler;
            item.ptr = cast::to_void(wf_data->id);
            m_panel_items.push_back(std::move(item));
        }
    }
}

void PanelWfList::handle_dirty(const PanelItemState& dirty_item)
{
    LogRAssert(dirty_item.index < m_panel_items.size(), "Index out of bounds", );

    if (dirty_item.expanded)
    {
        rebuild_items();
        return;
    }

    if (m_labels_mode)
    {
        handle_dirty_labels(dirty_item);
        return;
    }

    if (m_deleted_mode)
    {
        handle_dirty_trash(dirty_item);
        return;
    }

#ifdef _DEBUG
    Log("Dirty. idx=" << dirty_item.index << ", rc=" << dirty_item.rclicked << " lc=" << dirty_item.lclicked << ", ico=" << dirty_item.icon_clicked << ", xp=" << dirty_item.expanded);
#endif

    auto& item = m_panel_items[dirty_item.index];
    const wid id = cast::from_ptr<wid>(item.ptr);

    if (dirty_item.lclicked)
    {
        g::pFlow->wf_activate(id);
    }
    else if (dirty_item.renamed)
    {
        g::pFlow->wf_rename(id, item.name);
        g::pFlow->host_handle_renaming(id, 0);
    }
    else if (dirty_item.icon_clicked && !dirty_item.lreleased)
    {
        rebuild_items();
    }
    else if (!dirty_item.lreleased)
    {
        LogError("Unhandled dirty state");
    }
}

void PanelWfList::rebuild_labels()
{
    const auto on_hover_handler = [](PanelListItem& item, bool& handler_processed)
    {
        bool dirty = false;
        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        {            
            g::pWfsManager->remove_label(item.label);
            dirty = true;
            handler_processed = true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_F2))
        {
            item.name_backup = item.name;
            item.renaming = true;
            item.need_focus = true;
            handler_processed = true;
        }

        return dirty;
    };

    const auto on_item_handler = [&new_label_id = m_new_label]
        (PanelListItem& item, bool /*l_clicked*/, bool /*l_dbl_clicked*/, bool /*l_released*/, bool /*r_clicked*/, bool r_released, bool& handler_processed)
    {
        bool dirty = false, close = false;
        if (!item.continue_processing_item_handler && r_released)
        {
            ImGui::OpenPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_LabelsMenu)]);
            handler_processed = true;
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(ImGui::GetContentRegionAvail().x, 200));
        if (ImGui::BeginPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelWfList_LabelsMenu)]))
        {
            handler_processed = true;
            if (ImGui::MenuItem("Add new label"))
            {
                new_label_id = g::pWfsManager->create_label();
                close = dirty = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Rename label"))
            {
                item.name_backup = item.name;
                item.renaming = true;
                item.need_focus = true;
                close = true;
            }
            if (ImGui::MenuItem("Delete label"))
            {
                g::pWfsManager->remove_label(item.label);
                close = dirty = true;
            }

            item.continue_processing_item_handler = !close;
            if (close)
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return dirty;
        }
        else
        item.continue_processing_item_handler = false;

        return dirty;
    };

    auto& labels = g::pWfsManager->get_labels();

    m_panel_items.clear();
    m_panel_items.reserve(labels.size());

    for (auto& label : labels)
    {
        PanelListItem item;
        item.can_dblclick = true;
        item.name = label.name;
        item.label = label.id;
        item.on_item_handler = on_item_handler;
        item.on_hover_handler = on_hover_handler;
        if (m_new_label == label.id)
        {
            item.renaming = true;
            item.need_focus = true;
            item.name_backup = item.name;
            m_new_label = 0;
        }
        m_panel_items.push_back(std::move(item));
    }
}

void PanelWfList::handle_dirty_labels(const PanelItemState& dirty_item)
{
#ifdef _DEBUG
    Log("Dirty label: idx=" << dirty_item.index << ", rc=" << dirty_item.rclicked << " lc=" << dirty_item.lclicked << ", ico=" << dirty_item.icon_clicked << ", xp=" << dirty_item.expanded);
#endif

    if (dirty_item.dblclicked)
    {
        LogRAssert(dirty_item.index < m_panel_items.size(), "Index out of bounds", );

        auto& panel_item = m_panel_items[dirty_item.index];
        panel_item.renaming = true;
        panel_item.need_focus = true;
        panel_item.name_backup = panel_item.name;
    }
    else if (dirty_item.item_clicked || dirty_item.item_handled || (dirty_item.icon_clicked && !dirty_item.lreleased))
    {
        rebuild_items();
    }
    else if (dirty_item.renamed)
    {
        auto& panel_item = m_panel_items[dirty_item.index];
        if (!g::pWfsManager->rename_label(panel_item.label, panel_item.name))
            panel_item.name = panel_item.name_backup;
        panel_item.name_backup = {};
        rebuild_items();
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
        item.icon_can_click = true;
        item.can_lclick = true;
        item.ptr = cast::to_void(wf_data->id);
        m_panel_items.push_back(std::move(item));
    }
}

void PanelWfList::handle_dirty_trash(const PanelItemState& dirty_item)
{
#ifdef _DEBUG
    Log("Dirty trash: idx=" << dirty_item.index << ", rc=" << dirty_item.rclicked << " lc=" << dirty_item.lclicked << ", ico=" << dirty_item.icon_clicked << ", xp=" << dirty_item.expanded);
#endif

    if ((dirty_item.icon_clicked || dirty_item.lclicked) && !dirty_item.lreleased)
    {
        const wid id = cast::from_ptr<wid>(m_panel_items[dirty_item.index].ptr);
        g::pFlow->wf_restore(id);
    }
}

void PanelWfList::on_add_new_wf(PanelToolbar::Element& /*el*/)
{
    g::pFlow->wf_create();
}

void PanelWfList::rename_project() noexcept
{
    if (!g::pFlow->get_nodes_ready())
        return;

    for (auto& item : m_panel_items)
    {
        if (cast::from_ptr<wid>(item.ptr) == m_active)
        {
            item.renaming = true;
            item.need_focus = true;
            item.name_backup = item.name;
            break;
        }
    }
}

void PanelWfList::on_rename_wf(PanelToolbar::Element& /*el*/) noexcept
{
    rename_project();
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

void PanelWfList::on_show_tt(int32_t /*idx*/) noexcept
{
    if (m_deleted_mode)
    {
        ImGui::TextUnformatted("Undelete Project");
    }
}

} // namespace ui
