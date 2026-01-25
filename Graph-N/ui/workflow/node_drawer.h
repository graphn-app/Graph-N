
#pragma once

namespace data
{
class NodeInstance;
class PinInstance;
}

namespace ui
{

class NodeDrawer final
{
public:
    NodeDrawer();
    ~NodeDrawer();

    NodeDrawer(const NodeDrawer&) = delete;
    NodeDrawer& operator=(const NodeDrawer&) = delete;

    NodeDrawer(NodeDrawer&&);
    NodeDrawer& operator=(NodeDrawer&&);

    float draw(data::NodeInstance* node, float ideal_fit_w);

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

private:
    const ImU32 m_clr_node_header_bg;
    const ImU32 m_clr_node_header_text;
    const ImU32 m_clr_node_body_bg;
    const ImU32 m_clr_node_pin_text;
    const ImU32 m_clr_node_border;

    float m_curr_node_left_offset = 0.0f;
};


} // namespace ui
