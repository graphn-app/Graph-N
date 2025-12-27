
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/text_wrapper_ro.h"

namespace data
{
struct Node;
}

namespace ax::NodeEditor
{
struct EditorContext;
}

namespace ui
{

struct PanelNodeInspectorData final : public PanelCommonData
{
    ImU32 clr_descr_frame = 0;			// description frame
    ImU32 clr_selected_node = 0;		// selected node text
};

class PanelNodeInspector final : public PanelBase<PanelNodeInspectorData>
{
public:
    PanelNodeInspector();
    ~PanelNodeInspector() override = default;

    void init() override;

    void set_inspecting_node(data::Node* node);

private:
    DlgCode show(bool& focused) override;
    void show_description();
    void show_node_preview();

private:
    data::Node* m_inspecting_node = nullptr;
    bool m_node_changed = false;
    TextWrapperRO m_descr_wrapper;
    lid m_preview_node_id = 0;
    ax::NodeEditor::EditorContext* m_context = nullptr;
};


} // namespace ui