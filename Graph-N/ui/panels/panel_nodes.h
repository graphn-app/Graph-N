#pragma once
#include "ui/panels/panel_base.h"
#include "ui/node_drawer_ro.h"
#include "core/listeners.h"

namespace data
{
struct Node;
struct NodeItem;
class NodeInstanceRo;
class Workflow;
}

namespace ui
{

class PanelNodeInspector;

struct PanelNodesData final : public PanelCommonData
{
    enum Buffer
    {
        BuffFilter,
        BuffFilterLowered,

        _buf_count
    };
};

class PanelNodes final
    : public PanelEmptyWithDescr
    , public PanelWithList
    , public PanelToolbar
    , public PanelBase<PanelNodesData>
    , public listener::ComfyUiObjectValidityListener
{
public:
    PanelNodes(PanelNodeInspector* inspector);
    ~PanelNodes() noexcept override;

    void init() override;

    //ComfyUiObjectValidityListener
    void invalidate_comfyui_objects() noexcept override;
    void resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step step) override;

private:
    DlgCode show(bool& focused) override;

    void add_node_to_wf(data::NodeItem* node_item);

    void create_toolbar();

    void rebuild_items(bool force_expand_all = false);
    void rebuild_labels();
    void draw_popups();
    void handle_dirty(const PanelItemEvent& ev);
    void handle_dirty_labels(const PanelItemEvent& ev);

    void on_alloc_tt_data(int32_t idx);
    void on_release_tt_data(int32_t idx);
    void on_show_tt(int32_t idx);
    bool on_pre_tt(int32_t idx) noexcept;
    void on_post_tt(int32_t idx) noexcept;

    bool on_dnd_allow(int32_t idx) noexcept;
    void on_dnd_show(int32_t idx) noexcept;

    void on_labels_mode(PanelToolbar::Element& el);
    void on_new_label(PanelToolbar::Element& el);
    void on_expand_all(PanelToolbar::Element& el);
    void on_collapse_all(PanelToolbar::Element& el);
    void on_view(PanelToolbar::Element& el);
    void on_pre_expand_collapse(PanelToolbar::Element& el) noexcept;

private:
    PanelNodeInspector* m_inspector = nullptr;
    data::Workflow* m_wf = nullptr;
    PanelWithList::TtCallbacks m_tt_callbacks;
    PanelWithList::DndCallbacks m_dnd_callbacks;
    data::NodeInstanceRo* m_tt_node = nullptr;
    float m_tt_width = 0.0f;
    RoNodeDrawer m_tt_node_drawer;
    int32_t m_tt_idx = -1;
    std::vector<PanelListItem> m_panel_items;
    bool m_labels_mode = false;
    uint32_t m_new_label = 0;

    void* m_active_popup_ptr = nullptr;
    std::set<uint32_t> m_active_popup_labels;
    char m_new_label_buff[31] = {};
    uint32_t m_active_popup_label_id = 0;
};


} // namespace ui
