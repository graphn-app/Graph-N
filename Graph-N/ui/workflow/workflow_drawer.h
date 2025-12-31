
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
    void draw();

    data::Workflow* get_wf() { return m_wf; }

private:
    void draw_node(const data::NodeInstance* node);
    void draw_input_pin(const data::PinInstance* pin);
    void draw_input_widget(const data::PinInstance* pin);
    void draw_output_pin(const data::PinInstance* pin);
    void draw_link(const data::Link* link);

private:
    ax::NodeEditor::EditorContext* m_context = nullptr;
    data::Workflow* m_wf = nullptr;
};

} // namespace ui
