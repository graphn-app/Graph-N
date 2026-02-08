
#include "ui/panels/panel_models.h"
#include "core/managers/models_manager.h"
#include "ui/graph-n_flow.h"

namespace ui
{

PanelModels::PanelModels()
    : PanelBase(DlgObj::PanelModels, ICON_LC_COMPONENT " Model Library", ICON_LC_COMPONENT, DlgObjID[cast::to_szt(DlgObj::PanelModels)])
{}

PanelModels::~PanelModels()
{
    LogF;
}

void PanelModels::init()
{
    LogF;
    PanelBase::init();
    data()->buffer.resize(PanelModelsData::_buf_count);

    m_tt_callbacks.on_alloc_tt_data = std::bind(&PanelModels::on_alloc_tt_data, this, std::placeholders::_1);
    m_tt_callbacks.on_release_tt_data = std::bind(&PanelModels::on_release_tt_data, this, std::placeholders::_1);
    m_tt_callbacks.on_show_tt = std::bind(&PanelModels::on_show_tt, this, std::placeholders::_1);
    m_tt_callbacks.on_pre_tt = std::bind(&PanelModels::on_pre_tt, this, std::placeholders::_1);
    m_tt_callbacks.on_post_tt = std::bind(&PanelModels::on_post_tt, this, std::placeholders::_1);

    create_toolbar();
    rebuild_items();
}

void PanelModels::stop_all_threads()
{
    LogF;
}

void PanelModels::create_toolbar()
{
    clear_toolbar();

    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_LIST_CHEVRONS_UP_DOWN;
        button.tooltip = "Expand All";
        button.on_click = std::bind(&PanelModels::on_expand_all, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_LIST_CHEVRONS_DOWN_UP;
        button.tooltip = "Collapse All";
        button.on_click = std::bind(&PanelModels::on_collapse_all, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.empty_indent = true;
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_FOLDER_SYNC;
        button.tooltip = "Refresh Models List";
        button.on_click = std::bind(&PanelModels::on_sync, this, std::placeholders::_1);
        button.on_preview = std::bind(&PanelModels::on_pre_sync, this, std::placeholders::_1);
        add_button(std::move(button));
    }        
    
    finalize_toolbar();
}

DlgCode PanelModels::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        if (g::pModelsManager->get_models().empty())
        {
            show_empty_panel_descr(
                ICON_LC_INFO " Model Library",
                {},
                {
                    "Displays available Checkpoints,",
                    "LoRAs, VAEs, and other models.",
                    "Make sure " APP_CAPTION " is connected",
                    "to the " APP_COMFYUI " server.",
                    "Click model to quickly create loader node",
                }                
            );
            return end_panel(true);
        }

        draw_toolbar();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGuiEx::InputTextWithHint("##filtermodels", "filter models", &data()->buffer[PanelModelsData::BuffFilter],
            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EscapeClearsAll))
        {
            data()->buffer[PanelModelsData::BuffFilterLowered] = tools::str::to_lower_ascii(data()->buffer[PanelModelsData::BuffFilter]);
            Log("Filter by: " << data()->buffer[PanelModelsData::BuffFilterLowered]);
            g::pModelsManager->calc_number_inside(tools::str::trim_this(data()->buffer[PanelModelsData::BuffFilterLowered]));
            rebuild_items(true);
        }

        ImGuiEx::Spacing(3);
        if (ImGui::BeginChild(DlgObjID[cast::to_szt(DlgObj::PanelModels_ItemsList)], ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            if (m_prev_model_items_count != g::pModelsManager->get_models().size())
                rebuild_items();

            PanelItemState state;
            if (draw_list(m_panel_items, state, &m_tt_callbacks))
                handle_dirty(state);
        }
        ImGui::EndChild();

    }
    return end_panel(opened);
}

void PanelModels::rebuild_items(bool force_expand_all /*= false*/)
{
    LogF;

    std::set<data::ModelItem*> expanded_models;
    for (auto& item : m_panel_items)
    {
        if (item.expanded)
        {
            expanded_models.insert((data::ModelItem*)item.ptr);
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
    go_tree = [&go_tree, &expanded_models, force_expand_all, 
        is_filter_empty = data()->buffer[PanelModelsData::BuffFilter].empty()]
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
                        show_model = is_filter_empty || !is_filter_empty && model->models_inside > 0;
                }
                else
                {
                    show_model = show_favorited_models && model->starred || !show_favorited_models;
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
                    PanelListItem item;
                    item.depth = depth;
                    item.name = model->name;
                    item.name_p2 = std::to_string(model->models_inside);
                    item.can_expand = true;
                    item.expanded = force_expand_all || expanded_models.count(model.get()) > 0;
                    expanded = item.expanded;
                    item.icon = item.expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER;
                    const bool item_expanded = item.expanded;
                    item.ptr = model.get();
                    dest.push_back(std::move(item));
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
                item.icon_active = model->starred;
                item.can_lclick = item.icon_can_click = true;
                item.ptr = model.get();
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

    if (!m_panel_items.empty() || g::pModelsManager->has_service_components())
        add_separator("Models");
    go_tree(models, m_panel_items, 0, true, false, false);

    if (g::pModelsManager->has_service_components())
    {
        add_separator("Service Components");
        go_tree(models, m_panel_items, 0, false, false, true);
    }
}

void PanelModels::handle_dirty(const PanelItemState& dirty_item)
{
    if (dirty_item.index >= m_panel_items.size())
    {
        LogError(ExcepCategory::Logic, "Index out of bounds");
    }

    Log("Dirty: idx=" << dirty_item.index << ", rc=" << dirty_item.rclicked << " lc=" << dirty_item.lclicked << ", ico=" << dirty_item.icon_clicked << ", xp=" << dirty_item.expanded);

    if (dirty_item.expanded)
    {
        rebuild_items();
        return;
    }
    if (dirty_item.icon_clicked)
    {
        auto* model = cast::ptrs<data::ModelItem>(m_panel_items[dirty_item.index].ptr);
        model->starred = !model->starred;
        g::pOptsManager->model_starred_save(model->id, model->starred);
        rebuild_items();
        return;
    }

    auto& panel_item = m_panel_items[dirty_item.index];
    data::ModelItem* model_item = cast::ptrs<data::ModelItem>(panel_item.ptr);

    if (dirty_item.lclicked)
    {
        add_model_to_wf(model_item);
    }
    else
    {
        LogError(ExcepCategory::Logic, "Unhandled dirty state");
    }
}

void PanelModels::on_alloc_tt_data(int32_t idx)
{
    if (m_panel_items[idx].can_expand || nullptr == m_panel_items[idx].ptr)
        return;

    auto* model = cast::ptrs<data::ModelItem>(m_panel_items[idx].ptr);
    if (model->image_path.empty())
        return;

    //if (m_tt_model != nullptr)
    //    m_wf->remove_node(m_tt_node);
    //m_tt_idx = idx;
    //m_tt_node = m_wf->add_node(((data::NodeItem*)m_panel_items[idx].ptr)->node);
    //m_tt_width = m_tt_node->calc_node_width(true /*ro*/);
}

void PanelModels::on_release_tt_data(int32_t /*idx*/)
{/*
    if (m_tt_node != nullptr)
        m_wf->remove_node(m_tt_node);
    m_tt_node = nullptr;
    m_tt_idx = -1;*/
}

void PanelModels::on_show_tt(int32_t /*idx*/)
{/*
    if (m_tt_node && idx == m_tt_idx)
        m_tt_node_drawer.draw(m_tt_node, true, m_tt_width);*/
}

bool PanelModels::on_pre_tt(int32_t idx)
{
    if (m_panel_items[idx].can_expand || nullptr == m_panel_items[idx].ptr)
        return false;

    auto* model = cast::ptrs<data::ModelItem>(m_panel_items[idx].ptr);
    if (model->descr.empty() && !tools::path::exists(model->image_path))
        return false;

    return true;
}

void PanelModels::on_post_tt(int32_t /*idx*/)
{
    ImGui::PopStyleColor(2);
}

void PanelModels::add_model_to_wf(data::ModelItem* /*node_item*/)
{
}

void PanelModels::on_expand_all(PanelToolbar::Button& /*button*/)
{
    rebuild_items(true);
}

void PanelModels::on_collapse_all(PanelToolbar::Button& /*button*/)
{
    for (auto& item : m_panel_items)
    {
        item.expanded = false;
    }
    rebuild_items();
}

void PanelModels::on_sync(PanelToolbar::Button& /*button*/)
{
    g::pFlow->reconnect();
}

void PanelModels::on_pre_sync(PanelToolbar::Button& button)
{
    button.enabled = g::pFlow->get_online();    
}

} // namespace ui
