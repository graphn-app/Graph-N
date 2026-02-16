
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/helpers/node_drawer_ro.h"
#include "core/listeners.h"

namespace data
{
struct Node;
struct NodeItem;
class NodeInstance;
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

class PanelNodes final :
    public PanelEmptyWithDescr,
    public PanelWithList,
    public PanelToolbar,
    public PanelBase<PanelNodesData>,
    public listener::ComfyUiObjectValidityListener
{
public:
    PanelNodes(PanelNodeInspector* inspector);
    ~PanelNodes() override;

    void init() override;

    //ComfyUiObjectValidityListener
    void invalidate_comfyui_objects() override;
    void finalize_resolve_comfyui_objects() override;

private:
    DlgCode show(bool& focused) override;

    void add_node_to_wf(data::NodeItem* node_item);

    void create_toolbar();

    void rebuild_items(bool force_expand_all = false);
    void rebuild_labels();

    void handle_dirty(const PanelItemState& dirty_item);
    void handle_dirty_labels(const PanelItemState& dirty_item);

    void on_alloc_tt_data(int32_t idx);
    void on_release_tt_data(int32_t idx);
    void on_show_tt(int32_t idx);
    bool on_pre_tt(int32_t idx);
    void on_post_tt(int32_t idx);

    void on_labels_mode(PanelToolbar::Element& el);
    void on_new_label(PanelToolbar::Element& el);
    void on_expand_all(PanelToolbar::Element& el);
    void on_collapse_all(PanelToolbar::Element& el);
    void on_view(PanelToolbar::Element& el);
    void on_sync(PanelToolbar::Element& el);
    void on_pre_expand_collapse(PanelToolbar::Element& el);
    void on_pre_sync(PanelToolbar::Element& el);

private:
    PanelNodeInspector* m_inspector = nullptr;
    data::Workflow* m_wf = nullptr;
    PanelWithList::TtCallbacks m_tt_callbacks;
    data::NodeInstance* m_tt_node = nullptr;
    float m_tt_width = 0.0f;
    RoNodeDrawer m_tt_node_drawer;
    int32_t m_tt_idx = -1;
    std::vector<PanelListItem> m_panel_items;
    bool m_labels_mode = false;
    uint32_t m_new_label = 0;
};


} // namespace ui
