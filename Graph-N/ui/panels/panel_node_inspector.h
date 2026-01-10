
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/text_wrapper_ro.h"
#include "ui/panels/ro_node_drawer.h"
#include "core/managers/comfyui_object_validity_listener.h"

namespace data
{
struct Node;
class NodeInstance;
class PinInstance;
class Workflow;
}

namespace ui
{

struct PanelNodeInspectorData final : public PanelCommonData
{
    ImU32 clr_descr_frame = 0;			// description frame
    ImU32 clr_selected_node = 0;		// selected node text
};

class PanelNodeInspector final :
    public PanelBase<PanelNodeInspectorData>,
    public mngr::ComfyUiObjectValidityListener
{
public:
    PanelNodeInspector();
    ~PanelNodeInspector() override;

    void init() override;

    void add_inspecting_node(data::Node* node);
    void remove_inspecting_node(data::Node* node);
    void reset_inspecting_nodes();

    //ComfyUiObjectValidityListener
    void invalidate_comfyui_objects() override;

private:
    DlgCode show(bool& focused) override;
    void show_nodes();
    void show_description(TextWrapperRO& descr_wrapper, float max_w);

private:
    data::Workflow* m_wf = nullptr;

    struct Data final
    {
        data::NodeInstance* node = nullptr;
        RoNodeDrawer drawer;
        float ideal_size = 0.0f;
        TextWrapperRO descr_wrapper;
    };
    std::vector<Data> m_inspecting;
};


} // namespace ui
