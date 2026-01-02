
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/text_wrapper_ro.h"
#include "core/managers/comfyui_object_validity_listener.h"

namespace data
{
struct Node;
}

namespace ui
{
class WorkflowDrawer;

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
    void show_description(TextWrapperRO& descr_wrapper);

    struct Data;
    void show_preview(Data& data);

private:
    struct Data final
    {
        data::Node* node = nullptr;
        TextWrapperRO descr_wrapper;
        WorkflowDrawer* wf = nullptr;
    };
    std::map<hid, Data> m_inspecting;	// key: node id
};


} // namespace ui
