#pragma once
#include "ui/panels/panel_base.h"
#include "core/listeners.h"

namespace data
{
struct Node;
struct ModelItem;
}

namespace ui
{

struct PanelModelsData final : public PanelCommonData
{
    enum Buffer
    {
        BuffFilter,
        BuffFilterLowered,

        _buf_count
    };
};

class PanelModels final : 
    public PanelEmptyWithDescr,
    public PanelWithList,
    public PanelToolbar,
    public PanelBase<PanelModelsData>,
    public listener::ComfyUiObjectValidityListener
{
public:
    PanelModels();
    ~PanelModels() noexcept override;

    void init() override;
    void rebuild_items(bool force_expand_all = false);

    //ComfyUiObjectValidityListener
    void invalidate_comfyui_objects() noexcept override;
    void resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step step) override;

private:
    DlgCode show(bool& focused) override;

    void create_toolbar();
    void handle_dirty(const PanelItemState& dirty_item);

    void on_alloc_tt_data(int32_t idx);
    bool on_pre_tt(int32_t idx) noexcept;
    void on_show_tt(int32_t idx);

    void on_expand_all(PanelToolbar::Element& el);
    void on_collapse_all(PanelToolbar::Element& el);
    void on_sync(PanelToolbar::Element& el) noexcept;
    void on_pre_sync(PanelToolbar::Element& el) noexcept;

    void add_model_to_wf(data::Node* node, data::ModelItem* model);

private:
    PanelWithList::TtCallbacks m_tt_callbacks;
    std::vector<PanelListItem> m_panel_items;
    size_t m_prev_model_items_count = 0;
};


} // namespace ui
