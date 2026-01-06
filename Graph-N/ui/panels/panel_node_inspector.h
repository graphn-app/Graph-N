
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/text_wrapper_ro.h"
#include "core/managers/comfyui_object_validity_listener.h"

namespace data
{
struct Node;
class NodeInstance;
class PinInstance;
class Workflow;
}

namespace ui
{

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
    void show_description(TextWrapperRO& descr_wrapper, float max_w);
    void show_preview(data::NodeInstance* node_inst, float max_w);

    void draw_node_header(data::NodeInstance* node);
    void draw_input_pin(data::PinInstance* pin);
    void draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width, bool draw_control_after_generate = false);
    void draw_output_pin(data::PinInstance* pin);
    void draw_pin_icon(ImU32 color);
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
    const ImU32 m_clr_node_widget_pin;
    const ImU32 m_clr_node_pin_text;
    const ImU32 m_clr_node_border;

    data::Workflow* m_wf = nullptr;

    struct Data final
    {
        data::NodeInstance* node = nullptr;
        float ideal_size = 0.0f;
        TextWrapperRO descr_wrapper;
    };
    std::vector<Data> m_inspecting;	// key: node id
    float m_curr_node_left_offset = 0.0f;
};


} // namespace ui
