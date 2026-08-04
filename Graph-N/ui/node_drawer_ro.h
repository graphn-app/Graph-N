
#pragma once

namespace data
{
class NodeInstanceRo;
class PinInstance;
class Workflow;
}

namespace ui
{

class RoNodeDrawer final
{
public:
    RoNodeDrawer(data::Workflow* wf) noexcept;
    ~RoNodeDrawer() noexcept;

    RoNodeDrawer(const RoNodeDrawer&) = delete;
    RoNodeDrawer& operator=(const RoNodeDrawer&) = delete;

    RoNodeDrawer(RoNodeDrawer&&) noexcept;
    RoNodeDrawer& operator=(RoNodeDrawer&&) noexcept;

    float draw(data::NodeInstanceRo* node, bool tooltip_mode, float max_w);

private:
    void draw_node_header(const data::NodeInstanceRo* node) noexcept;
    void draw_input_pin(data::PinInstance* pin) noexcept;
    void draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_output_pin(data::PinInstance* pin) noexcept;
    void draw_pin_icon(ImU32 color) noexcept;
    void draw_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_int(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_float(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_number(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_str_ml(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_combo(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_separator(float offset, float width) noexcept;

private:
    const ImU32 m_clr_node_header_bg;
    const ImU32 m_clr_node_header_text;
    const ImU32 m_clr_node_body_bg;
    const ImU32 m_clr_node_pin_text;
    const ImU32 m_clr_node_border;

    data::Workflow* m_wf = nullptr;
    data::NodeInstanceRo* m_node = nullptr;
    bool m_tt_mode = false;
    float m_curr_node_left_offset = 0.0f;
};


} // namespace ui
