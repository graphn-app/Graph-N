
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

class PanelNodeInspector final : public PanelBase<PanelNodeInspectorData>
{
public:
    PanelNodeInspector();
    ~PanelNodeInspector() override;

    void init() override;

    void add_inspecting_node(data::Node* node);
    void remove_inspecting_node(data::Node* node);
    void reset_inspecting_nodes();

private:
    DlgCode show(bool& focused) override;
    void show_nodes();
    void show_description(TextWrapperRO* descr_wrapper);
    void show_preview(Workflow* wf);

private:
    struct Data final
    {
        std::string node_name;
        std::unique_ptr<TextWrapperRO> descr_wrapper;
        Workflow* wf = nullptr;
    };

    std::map<hid, Data> m_inspecting;	// key: node id
};


} // namespace ui
