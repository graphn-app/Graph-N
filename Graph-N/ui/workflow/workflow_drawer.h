
#pragma once

namespace ax::NodeEditor
{
struct EditorContext;
}

namespace data
{
class Workflow;
class NodeInstance;
class PinInstance;
class Link;
}

namespace ui
{

class WorkflowDrawer final
{
public:
    WorkflowDrawer();
    ~WorkflowDrawer();

    NO_COPY_MOVE(WorkflowDrawer);

    void init(data::Workflow* wf);
    void draw(float max_w);

    data::Workflow* get_wf() { return m_wf; }

private:
    void draw_node(data::NodeInstance* node);
    void draw_link(const data::Link* link);

private:
    ax::NodeEditor::EditorContext* m_context = nullptr;
    data::Workflow* m_wf = nullptr;
    const ImU32 m_clr_node_header_bg;
    const ImU32 m_clr_node_header_text;
    const ImU32 m_clr_node_body_bg;
    const ImU32 m_clr_node_Pin_text;
    const ImU32 m_clr_node_border;
};

} // namespace ui
