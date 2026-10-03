#include "ui/panels/panel_models.h"
#include "core/managers/models_manager.h"
#include "core/managers/nodes_manager.h"
#include "ui/helpers/textures.h"
#include "ui/graph-n_flow.h"
#include "ui/helpers/font_guard.h"
#include "main.h"

namespace ui
{

PanelModels::PanelModels()
    : PanelBase(DlgObj::PanelModels, "Model Library")
{
    g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::Invalidate, this);
    g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::ResolvePanels, this);
}

PanelModels::~PanelModels() noexcept
{
    if (g::pGraphN != nullptr)
        g::pGraphN->unsubscribe_comfyui_validity_state(this);
}

void PanelModels::init()
{
    PanelBase::init();
    dlg_data()->buffer.resize(PanelModelsData::_buf_count);

    m_tt_callbacks.on_alloc_tt_data = [this](int32_t idx) { on_alloc_tt_data(idx); };
    m_tt_callbacks.on_show_tt = [this](int32_t idx) { on_show_tt(idx); };
    m_tt_callbacks.on_pre_tt = [this](int32_t idx) noexcept { return on_pre_tt(idx); };

    m_favorited = g::pOptsManager->items_model_favs_load();

    create_toolbar();
    rebuild_items();
}

void PanelModels::create_toolbar()
{
    clear_toolbar();

    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_EYE_OFF;
        el.tooltip = "Toggle Show/Hide empty categories";
        el.on_click = [this](PanelToolbar::Element& el)
        {
            m_show_empty = !m_show_empty;
            el.icon = m_show_empty ? ICON_LC_EYE : ICON_LC_EYE_OFF;
            rebuild_items();
        };
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
        el.icon = ICON_LC_LIST_CHEVRONS_UP_DOWN;
        el.tooltip = "Expand All";
        el.on_click = [this](PanelToolbar::Element& /*el*/) { rebuild_items(true); };
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Button;
        el.icon = ICON_LC_LIST_CHEVRONS_DOWN_UP;
        el.tooltip = "Collapse All";
        el.on_click = [this](PanelToolbar::Element& /*el*/)
        {
            for (auto& item : m_panel_items) { item.expanded = false; }
            rebuild_items();
        };
        add_toolbar_element(std::move(el), false);
    }
    {
        PanelToolbar::Element el;
        el.type = PanelToolbar::Element::Type::Empty;
        add_toolbar_element(std::move(el), false);
    }    
    
    finalize_toolbar(false /*separator*/);
}

void PanelModels::invalidate_comfyui_objects() noexcept
{
    m_panel_items.clear();
    m_prev_model_items_count = 0;
}

void PanelModels::resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step /*step*/)
{
    rebuild_items();
}

DlgCode PanelModels::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        if (g::pModelsManager->get_models().empty()) [[unlikely]]
        {
            show_empty_panel_descr(
                ICON_LC_INFO " Model Library",
                {
                    ICON_LC_UNPLUG,
                    "Waiting for connection.",
                    "Model definitions are unavailable."
                },
                {
                    "Displays available Checkpoints,",
                    "LoRAs, VAEs, and other models.",
                    "Click model to quickly",
                    "create loader node",
                }                
            );
            return end_panel(true);
        }

        draw_toolbar();
        ImGui::Spacing();

        if (m_appeared)
            ImGui::SetKeyboardFocusHere();

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGuiEx::InputTextWithHint("##filtermodels", "filter models", &dlg_data()->buffer[PanelModelsData::BuffFilter],
            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EscapeClearsAll)) [[unlikely]]
        {
            dlg_data()->buffer[PanelModelsData::BuffFilterLowered] = tools::str::to_lower_ascii(dlg_data()->buffer[PanelModelsData::BuffFilter]);
            Log("Filter by: " << dlg_data()->buffer[PanelModelsData::BuffFilterLowered]);
            g::pModelsManager->calc_number_inside(tools::str::trim_this(dlg_data()->buffer[PanelModelsData::BuffFilterLowered]));
            rebuild_items(true);
        }

        ImGuiEx::Spacing(3);
        if (ImGui::BeginChild(DlgObj_IID[cast::to_szt(DlgObj::PanelModels_ItemsList)], ImVec2(ImGui::GetContentRegionAvail().x, 0))) [[likely]]
        {
            if (m_prev_model_items_count != g::pModelsManager->get_models().size())
                rebuild_items();

            PanelItemEvent ev;
            if (draw_list(m_panel_items, ev, &m_tt_callbacks))
                handle_dirty(ev);
            draw_popups();
        }
        ImGui::EndChild();

    }
    return end_panel(opened);
}

void PanelModels::rebuild_items(bool force_expand_all /*= false*/)
{
    std::set<data::ModelItem*> expanded_models;
    for (auto& item : m_panel_items)
    {
        if (item.expanded)
        {
            expanded_models.insert((data::ModelItem*)item.ext_data.ptr);
        }
    }

    m_panel_items.clear();
    m_prev_model_items_count = g::pModelsManager->get_models().size();
    m_panel_items.reserve(m_prev_model_items_count * 2);

    const auto add_separator = [&items = m_panel_items](const char* sep_name)
    {
        PanelListItem item;
        item.is_separator = true;
        item.name = sep_name;
        items.push_back(std::move(item));
    };

    std::function<void(const std::vector<std::unique_ptr<data::ModelItem>>&, std::vector<PanelListItem>&, int32_t, bool, bool, bool)> go_tree;
    go_tree = [this, &go_tree, &expanded_models, force_expand_all, 
        is_filter_empty = dlg_data()->buffer[PanelModelsData::BuffFilter].empty()]
        (const std::vector<std::unique_ptr<data::ModelItem>>& models, std::vector<PanelListItem>& dest, int32_t depth,
            bool show_main_models, bool show_favorited_models, bool show_service_components)
    {
        for (auto& model : models)
        {
            bool show_model = !model->filtered;
            if (show_model)
            {
                if (!model->is_model)
                {
                    if (show_model && show_main_models && model->service_component)
                        show_model = false;
                    if (show_model && show_service_components && !model->service_component)
                        show_model = false;
                    if (show_model)
                        show_model = is_filter_empty || (!is_filter_empty && model->models_inside > 0);
                }
                else
                {
                    show_model = show_favorited_models && m_favorited.has_item(model->hash) || !show_favorited_models;
                    if (show_model && (show_main_models && model->service_component || show_service_components && !model->service_component))
                        show_model = false;
                }
            }
            if (!show_model)
                continue;

            if (!model->is_model)
            {
                bool expanded = false;
                if (!show_favorited_models)
                {
                    if (model->models_inside > 0 || m_show_empty)
                    {
                        PanelListItem item;
                        item.depth = depth;
                        item.name = model->name;
                        item.name_label = std::to_string(model->models_inside);
                        item.can_expand = true;
                        item.expanded = (force_expand_all || expanded_models.contains(model.get()));
                        expanded = item.expanded;
                        item.icon = item.expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER;
                        item.ext_data.ptr = model.get();
                        dest.push_back(std::move(item));
                    }
                }

                if (show_favorited_models || expanded)
                    go_tree(model->children, dest, depth + 1, show_main_models, show_favorited_models, show_service_components);
            }
            else
            {
                PanelListItem item;
                item.depth = depth;
                item.name = model->name;
                item.icon = ICON_LC_STAR;
                item.icon_always = m_favorited.has_item(model->hash);
                if (!model->service_component)
                {
                    item.icon_can_click = true;
                }
                item.ext_data.ptr = model.get();
                dest.push_back(std::move(item));
            }
        }
    };

    const auto& models = g::pModelsManager->get_models();

    std::vector<PanelListItem> fav;
    go_tree(models, fav, 0, false, true, false);

    if (!fav.empty())
    {
        add_separator("Favorited");
        for (auto& item : fav)
        {
            item.depth = 0;
            m_panel_items.push_back(std::move(item));
        }
    }

    bool displ_cats = true;
    if (0 == g::pModelsManager->service_components_num().first)
        displ_cats = false;
    else if (0 == g::pModelsManager->service_components_num().second && !m_show_empty)
        displ_cats = false;

    if (!fav.empty() || displ_cats)
        add_separator("Models");
    go_tree(models, m_panel_items, 0, true, false, false);

    if (displ_cats)
    {
        add_separator("Service Components");
        go_tree(models, m_panel_items, 0, false, false, true);
    }
}

void PanelModels::handle_dirty(const PanelItemEvent& event)
{
    LogRAssert(event.index < m_panel_items.size(), "Index out of bounds", );

    if (event.expand_toggled)
    {
        m_panel_items[event.index].expanded = !m_panel_items[event.index].expanded;
        rebuild_items();
        return;
    }

    auto& panel_item = m_panel_items[event.index];
    data::ModelItem* model_item = cast::ptrs<data::ModelItem>(panel_item.ext_data.ptr);

    if (event.icon_clicked && event.l_clicked)
    {
        m_favorited.toggle_item(model_item->hash);
        g::pOptsManager->items_model_favs_save(m_favorited);
        rebuild_items();
        return;
    }

    if (event.r_released)
    {
        m_active_popup_ptr = model_item;
        ImGui::OpenPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelModels_ItemMenu)]);
        return;
    }

    if (event.l_clicked)
    {
        add_model_to_wf(nullptr, model_item);
    }
}

void PanelModels::draw_popups()
{
    if (ImGui::BeginPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelModels_ItemMenu)], ImGuiWindowFlags_NoMove))
    {
        auto* model = cast::ptrs<data::ModelItem>(m_active_popup_ptr);
        if (!model) 
        {
            ImGui::EndPopup();
            return;
        }

        bool close = false;
        bool separator = false;

        if (!model->info.tags.empty())
        {
            separator = true;
            if (ImGui::MenuItem("Copy Tags to Clipboard"))
            {
                close = true;
                ImGui::SetClipboardText(model->info.tags.c_str());
            }
        }
        if (!model->info.trigger_phrase.empty())
        {
            separator = true;
            if (ImGui::MenuItem("Copy Trigger phrase to Clipboard"))
            {
                close = true;
                ImGui::SetClipboardText(model->info.trigger_phrase.c_str());
            }
        }
        if (model->copy_to_cb_only)
        {
            separator = true;
            if (ImGui::MenuItem("Copy name to Clipboard"))
            {
                close = true;
                ImGui::SetClipboardText(model->name.c_str());
            }
        }

        if (separator)
            ImGui::Separator();
        bool has_menu_items = separator;

        data::Node* open_node = nullptr;

        if (model->def_node != 0)
        {
            if (!model->def_node_installed) ImGui::BeginDisabled();
            ImGui::PushFont(g::pMain->get_bold_font());
            if (ImGui::MenuItem(model->def_node_name.c_str()))
            {
                close = true;
                open_node = g::pNodesManager->find_node(model->def_node);
            }
            ImGui::PopFont();
            if (!model->def_node_installed) ImGui::EndDisabled();
            has_menu_items = true;
        }

        for (auto node_id : model->nodes)
        {
            if (node_id == model->def_node)
                continue;
            data::Node* node = g::pNodesManager->find_node(node_id);
            if (node != nullptr && ImGui::MenuItem(node->name.c_str()))
            {
                close = true;
                open_node = node;
            }
            has_menu_items = true;
        }

        if (open_node != nullptr)
            add_model_to_wf(open_node, model);

        if (close || !has_menu_items)
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void PanelModels::on_alloc_tt_data(int32_t idx)
{
    if (m_panel_items[idx].can_expand || nullptr == m_panel_items[idx].ext_data.ptr)
        return;

    auto* model = cast::ptrs<data::ModelItem>(m_panel_items[idx].ext_data.ptr);
    if (model->info.empty())
        return;

    if (g::pTextures->get_image_alive(model->info.textures_id))
        return;

    model->info.textures_id = g::pTextures->load_image_from_path(model->info.image_path, 300, 300, 20);
}

bool PanelModels::on_pre_tt(int32_t idx) noexcept
{
    if (m_panel_items[idx].can_expand || nullptr == m_panel_items[idx].ext_data.ptr)
        return false;

    auto* model = cast::ptrs<data::ModelItem>(m_panel_items[idx].ext_data.ptr);
    if (model->info.empty())
        return false;

    return true;
}

void PanelModels::on_show_tt(int32_t idx)
{
    if (m_panel_items[idx].can_expand || nullptr == m_panel_items[idx].ext_data.ptr)
        return;

    auto* model = cast::ptrs<data::ModelItem>(m_panel_items[idx].ext_data.ptr);

    FontGuard fg(g::pMain->get_bold_font());
    ImGui::TextUnformatted(model->name.c_str(), model->name.c_str() + model->name.size());
    fg.pop();
    ImGui::Separator();
    ImGui::TextWrapped(model->info.descr.c_str(), model->info.descr.c_str() + model->info.descr.size());
    ImGui::Separator();
    ImGui::TextUnformatted(model->info.info.c_str(), model->info.info.c_str() + model->info.info.size());

    if (!model->info.tags.empty() || !model->info.trigger_phrase.empty())
    {
        ImGui::Separator();
        if (!model->info.tags.empty())
        {
            fg.push();
            ImGui::TextUnformatted("Tags: ");
            fg.pop();
            ImGui::TextUnformatted(model->info.tags.c_str(), model->info.tags.c_str() + model->info.tags.size());
        }
        if (!model->info.trigger_phrase.empty())
        {
            fg.push();
            ImGui::TextUnformatted("Trigger phrase: ");
            fg.pop();
            ImGui::TextUnformatted(model->info.trigger_phrase.c_str(), model->info.trigger_phrase.c_str() + model->info.trigger_phrase.size());
        }
    }

    if (model->info.textures_id != 0)
    {
        float w, h;
        std::optional<bool> valid;
        auto textures = g::pTextures->get_image(model->info.textures_id, valid, &w, &h);
        if (textures != nullptr)
            ImGui::Image(textures, ImVec2(w, h));
        else if (!valid.value_or(true))
            ImGui::TextUnformatted("Can't render image data");
    }
}

void PanelModels::add_model_to_wf(data::Node* node, data::ModelItem* model)
{
    if (nullptr == node)
        node = g::pNodesManager->find_node(model->def_node);
    if (node != nullptr)
        g::pFlow->wf_add_node_to_active_panel(node, model->file_name.empty() ? model->name : model->file_name);
}

} // namespace ui
