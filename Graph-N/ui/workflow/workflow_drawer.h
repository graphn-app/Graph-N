
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
    WorkflowDrawer(bool ro_mode);
    ~WorkflowDrawer();

    NO_COPY_MOVE(WorkflowDrawer);

    void init(data::Workflow* wf);
    void draw();

    data::Workflow* get_wf() { return m_wf; }

private:
    void draw_node(data::NodeInstance* node);
    void draw_input_pin(data::PinInstance* pin);
    void draw_input_widget(data::PinInstance* pin, float max_w, bool draw_control_after_generate = false);
    void draw_output_pin(data::PinInstance* pin);
    void draw_link(const data::Link* link);
    void draw_pin_icon(ImU32 color, bool connected, bool for_widget);
    void calc_node_width(data::NodeInstance* node);
    void draw_widget(data::PinInstance* pin, float offset, float max_w);
    void draw_widget_int(data::PinInstance* pin, float offset, float max_w);
    void draw_widget_uint(data::PinInstance* pin, float offset, float max_w);
    void draw_widget_float(data::PinInstance* pin, float offset, float max_w);
    void draw_widget_bool(data::PinInstance* pin, float offset, float max_w);
    void draw_widget_str(data::PinInstance* pin, float offset, float max_w);
    void draw_widget_str_ml(data::PinInstance* pin, float offset, float max_w);
    void draw_widget_num(data::PinInstance* pin, float offset, float max_w);
    void draw_widget_combo(data::PinInstance* pin, float offset, float max_w);
    void draw_widget_color(data::PinInstance* pin, float offset, float max_w);
    void draw_widget_control_after_generate(data::PinInstance* pin, float offset, float max_w);

private:
    ax::NodeEditor::EditorContext* m_context = nullptr;
    data::Workflow* m_wf = nullptr;
    bool m_ro_mode = false;
    const ImU32 m_clr_node_header_bg;
    const ImU32 m_clr_node_header_text;
    const ImU32 m_clr_node_body_bg;
    const ImU32 m_clr_node_widget_pin;
    const ImU32 m_clr_node_pin_text;
    const ImU32 m_clr_node_border;
};

} // namespace ui
