
#pragma once

namespace data
{
class NodeInstance;
class PinInstance;
}

namespace ui
{

class RoNodeDrawer final
{
public:
    RoNodeDrawer();
    ~RoNodeDrawer();

    RoNodeDrawer(const RoNodeDrawer&) = delete;
    RoNodeDrawer& operator=(const RoNodeDrawer&) = delete;

    RoNodeDrawer(RoNodeDrawer&&);
    RoNodeDrawer& operator=(RoNodeDrawer&&);

    float draw(data::NodeInstance* node, bool tooltip_mode, float max_w);

private:
    void draw_node_header(data::NodeInstance* node);
    void draw_input_pin(data::PinInstance* pin);
    void draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width, bool draw_control_after_generate = false);
    void draw_output_pin(data::PinInstance* pin);
    void draw_Pin_icon(ImU32 color);
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
    const ImU32 m_clr_node_Pin_text;
    const ImU32 m_clr_node_border;

    data::NodeInstance* m_node = nullptr;
    bool m_tt_mode = false;
    float m_curr_node_left_offset = 0.0f;
};


} // namespace ui
