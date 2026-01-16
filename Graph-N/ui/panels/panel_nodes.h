
#pragma once
#include "ui/panels/panel_base.h"
#include "core/managers/comfyui_object_validity_listener.h"
#include "ui/panels/ro_node_drawer.h"

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
        BuffCount
    };

    std::set<hid> open_items;

    ImU32 clr_hover_group = 0;			// hovered group frame
    ImU32 clr_hover_node = 0;			// hovered node frame
    ImU32 clr_selected_node = 0;		// selected node text
    ImU32 clr_hover_btn = 0;			// hovered button
    ImU32 clr_active_btn = 0;			// active button

    std::set<lid> selected_nodes;
};

class PanelNodes final :
    public PanelBase<PanelNodesData>,
    public mngr::ComfyUiObjectValidityListener
{
public:
    PanelNodes(PanelNodeInspector* inspector);
    ~PanelNodes() override;

    void init() override;

    //ComfyUiObjectValidityListener
    void invalidate_comfyui_objects() override;

private:
    DlgCode show(bool& focused) override;
    void show_tree_view();
    void show_plain_view();

    void show_cfg_buttons();
    void expand_all();
    void collapse_all();
    void test_node_clicks(data::Nodes* holder);
    void add_node_to_wf(data::Nodes* holder);
    void show_tooltip(data::Nodes* holder);

private:
    PanelNodeInspector* m_inspector = nullptr;
    data::Workflow* m_wf = nullptr;
    data::NodeInstance* m_tt_node = nullptr;
    float m_tt_width = 0.0f;
    RoNodeDrawer m_tt_node_drawer;
};


} // namespace ui
