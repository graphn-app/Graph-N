
#pragma once

namespace data
{
class NodeInstance;
class PinInstance;
}

namespace ax::NodeEditor
{
struct EditorContext;
}

namespace ImGuiExNe
{
struct DeferredAction;
}

namespace ui
{

class NodeDrawer final
{
public:
    NodeDrawer();
    ~NodeDrawer();

    NO_COPY_MOVE(NodeDrawer);

    inline ImGuiExNe::DeferredAction* get_deffered_action() const { return m_deffered_action.get(); }

    void init();
    void free();

    void pre_draw_all(const char* id);
    void post_draw_all();

    void pre_draw_node(data::NodeInstance* node);
    void draw_node(data::NodeInstance* node, float ideal_fit_w);
    void post_draw_node(data::NodeInstance* node);

    data::PinInstance* get_ml_widget_active() const;

private:
    void draw_node_header(data::NodeInstance* node);
    void draw_input_pin(data::PinInstance* pin);
    void draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width, bool draw_control_after_generate = false);
    void draw_output_pin(data::PinInstance* pin);
    void draw_pin_icon(data::PinInstance* pin, bool input);
    void draw_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_int(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_uint(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_float(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_str_ml(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_num(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_combo(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_control_after_generate(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_separator(float offset, float width);

    void handle_widget_active_state();

private:
    const ImU32 m_clr_node_header_bg;
    const ImU32 m_clr_node_header_text;
    const ImU32 m_clr_node_body_bg;
    const ImU32 m_clr_node_pin_text;
    const ImU32 m_clr_node_border;

    ax::NodeEditor::EditorContext* m_context = nullptr;
    std::unique_ptr<ImGuiExNe::DeferredAction> m_deffered_action;
    data::NodeInstance* m_active_node = nullptr;
    float m_curr_node_left_offset = 0.0f;

    // TODO: remove
    std::set<void*> m_ids_imgui;
    std::set<void*> m_ids_nodelib;
};


} // namespace ui
