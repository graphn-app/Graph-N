
#pragma once
#include "ui/panels/panel_base.h"
#include "core/managers/comfyui_object_validity_listener.h"

namespace data
{
struct Node;
}

namespace ui
{

class Workflow;
class TextWrapperRO;

struct PanelNodeInspectorData final : public PanelCommonData
{
    ImU32 clr_descr_frame = 0;			// description frame
    ImU32 clr_selected_node = 0;		// selected node text
};

class PanelNodeInspector final : public PanelBase<PanelNodeInspectorData>, public mngr::ComfyUiObjectValidityListener
{
public:
    PanelNodeInspector();
    ~PanelNodeInspector() override;

    void init() override;

    void add_inspecting_node(data::Node* node);
    void remove_inspecting_node(data::Node* node);

    // DataInvalidationHandler
    void invalidate_comfyui_objects() override;
    void resolve_comfyui_objects() override;

private:
    DlgCode show(bool& focused) override;
    void show_nodes();

private:
    struct Data final
    {
        hid node_id = 0;
        std::unique_ptr<TextWrapperRO> descr_wrapper;
        Workflow* wf = nullptr;
    };

    std::map<hid, Data> m_inspecting;	// key: node id
};


} // namespace ui
