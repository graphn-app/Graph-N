
#pragma once
#include "imgui_node_editor_edtype.h"

namespace data
{
class NodeInstance;
class PinInstance;
class Link;
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

    void init(wid wf_id);
    void free();

    void pre_draw_all(const char* id, size_t nodes_count);
    void post_draw_all(const std::function<bool()>& init_menu_handler, const std::function<bool()>& show_menu_handler);

    void pre_draw_node(data::NodeInstance* node);
    void draw_node(data::NodeInstance* node, float node_w, size_t node_idx);
    void draw_group(data::NodeInstance* node, size_t node_idx);
    void post_draw_node(data::NodeInstance* node);

    void draw_links(const std::vector<std::unique_ptr<data::Link>>& links);
    bool query_links();

    void locate_node(data::NodeInstance* node);

    inline ImGuiExNe::DeferredAction* get_deffered_action() const { return m_deffered_action.get(); }
    data::PinInstance* get_ml_widget_active() const;

    ImVec2 get_mouse_click_pos() const;
    bool get_bg_menu_active() const;

    data::PinInstance* get_hovered_pin() const;
    data::NodeInstance* get_hovered_node() const;
    data::Link* get_hovered_link() const;

    std::set<data::Link*> get_selected_links() const;
    std::set<data::NodeInstance*> get_selected_nodes() const;
    bool is_node_selected(const EdType& node) const;
    void select_node(const EdType& node, bool append_to_selection) const;

    std::set<data::NodeInstance*> get_nodes_in_group(data::NodeInstance* group, const std::vector<data::NodeInstance*>& all_nodes) const;
    ImRect calc_group_rect(const ImRect& node_rc) const;

    void pin_node(const EdType& node, bool pin) const;
    void rename_node(data::NodeInstance* node);    

    void delete_node(const EdType& node) const;

    inline ImU32 get_default_node_color() const { return m_defclr_node_bg; }

private:
    void draw_node_header();
    void draw_input_pin(data::PinInstance* pin);
    void draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width, bool draw_control_after_generate = false);
    void draw_output_pin(data::PinInstance* pin);
    void draw_pin(data::PinInstance* pin, bool force_default_shape, bool align_to_widget);
    void draw_pin_shape(data::PinInstance* pin, bool force_default, bool align_to_widget);
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
    void draw_widget_separator(float offset, bool& opened, bool& temp_open);

    void handle_widget_active_state();
    bool handle_node_resize();

private:
    wid m_wf = 0;
    ax::NodeEditor::EditorContext* m_context = nullptr;

    data::NodeInstance* m_active_node = nullptr;
    const char* m_active_tt = nullptr;

    ImU32 m_defclr_node_bg;
    ImU32 m_defclr_node_header_bg;
    ImU32 m_clr_node_header_text;
    const ImU32 m_clr_node_pin_text;
    const float m_mode_alpha = 0.4f;

    data::NodeInstance* m_renaming_node = nullptr;
    std::string m_renaming_buff;
    bool m_need_renaming_focus = false;

    std::unique_ptr<ImGuiExNe::DeferredAction> m_deffered_action;
    float m_curr_node_left_offset = 0.0f;
    float m_curr_node_top_offset = 0.0f;
    data::PinInstance* m_querying_pin = nullptr;

    int32_t m_now_resizing = -1;
    const float m_grip_area_sz = 15.0f;
    std::vector<ImRect> m_resizing_grip_positions;
    std::vector<float> m_resizing_w;

#ifdef _DEBUG
    std::set<void*> m_ids_imgui;
    std::set<void*> m_ids_nodelib;
#endif
};


} // namespace ui
