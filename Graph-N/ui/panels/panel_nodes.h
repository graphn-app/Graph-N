
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/panels/ro_node_drawer.h"
#include "core/managers/comfyui_object_validity_listener.h"

namespace data
{
struct Node;
struct Nodes;
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
    public mngr::ComfyUiObjectValidityListener
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

    void add_node_to_wf(data::Nodes* holder);

    void rebuild_items(bool force_expand_all = false);
    void handle_dirty(const PanelItemState& dirty_item);

    void on_alloc_tt_data(int32_t idx);
    void on_release_tt_data(int32_t idx);
    void on_show_tt(int32_t idx);
    void on_pre_tt(int32_t idx);
    void on_post_tt(int32_t idx);

    void on_expand_all(PanelToolbar::Button& button);
    void on_collapse_all(PanelToolbar::Button& button);
    void on_view(PanelToolbar::Button& button);
    void on_sync(PanelToolbar::Button& button);
    void on_pre_expand_collapse(PanelToolbar::Button& button);
    void on_pre_sync(PanelToolbar::Button& button);

private:
    PanelNodeInspector* m_inspector = nullptr;
    data::Workflow* m_wf = nullptr;
    std::vector<PanelListItem> m_panel_items;
    PanelWithList::TtCallbacks m_tt_callbacks;
    data::NodeInstance* m_tt_node = nullptr;
    float m_tt_width = 0.0f;
    RoNodeDrawer m_tt_node_drawer;
    int32_t m_tt_idx = -1;
};


} // namespace ui
