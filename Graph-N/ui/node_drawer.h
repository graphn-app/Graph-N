
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

namespace data
{
class Workflow;
class WorkflowAction;
}

namespace ui
{


class NodeDrawer final
{
public:
    NodeDrawer(data::Workflow* wf);
    ~NodeDrawer();

    NO_COPY_MOVE(NodeDrawer);

    void init(blob::Blob&& state);
    void free();

    void pre_draw_all(const char* id, size_t nodes_count);
    void post_draw_all(const std::function<bool()>& init_menu_handler, const std::function<bool()>& show_menu_handler, const std::function<bool()>& move_handler);

    void pre_draw_node(data::NodeInstance* node);
    void draw_node(data::NodeInstance* node, float node_w, size_t node_idx);
    void draw_group(data::NodeInstance* node, size_t node_idx);
    void post_draw_node(data::NodeInstance* node);

    void draw_links(const std::vector<std::unique_ptr<data::Link>>& links);
    void query_new_item(const std::function<void(data::PinInstance*)>& unlink_handler, const std::function<void(data::PinInstance*, const ImVec2&)>& new_node_handler);

    void locate_node(data::NodeInstance* node);

    inline ImGuiExNe::DeferredAction* get_deffered_action() const { return m_deffered_action.get(); }
    data::PinInstance* get_ml_widget_active() const;

    ImVec2 get_mouse_pos() const;
    bool get_bg_menu_active(bool& rclick, bool& dbl_click) const;

    data::PinInstance* get_hovered_pin() const;
    data::NodeInstance* get_hovered_node() const;
    data::Link* get_hovered_link() const;
    
    std::set<data::NodeInstance*> get_selected_nodes() const;
    bool is_node_selected(const EdType& node) const;
    void select_node(const EdType& node, bool append_to_selection) const;

    std::set<data::Link*> get_selected_links() const;
    bool is_link_selected(const EdType& link) const;
    void select_link(const EdType& link, bool append_to_selection) const;

    void clear_selection();

    std::set<data::NodeInstance*> get_nodes_in_group(data::NodeInstance* group, const std::vector<std::unique_ptr<data::NodeInstance>>& all_nodes) const;
    ImRect calc_group_rect(const ImRect& node_rc) const;
    ImRect fit_group_rect(const data::NodeInstance* group, const std::set<data::NodeInstance*>& nodes) const;

    void pin_node(const EdType& node, bool pin) const;
    void rename_node(data::NodeInstance* node);
    void rename_pin(data::PinInstance* pin);
    void handle_node_collapsing(data::NodeInstance* node);

    void delete_node(const data::NodeInstance* node);
    void delete_link(const data::Link* link);

    inline ImU32 get_default_node_color() const { return m_defclr_node_bg; }

    float get_zoom() const;
    void set_zoom(float zoom);
    const ImRect& get_viewport_rect() const;
    void set_viewport_rect(const ImRect& rect);

    ImVec2 get_visible_pos(std::optional<ImVec2> add_offset) const;

    inline const blob::Blob& get_state() const { return m_state; }

private:
    void draw_node_header();
    void draw_collapsed_pins(bool input);
    void draw_input_pin(data::PinInstance* pin);
    void draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_output_pin(data::PinInstance* pin);
    void draw_pin(data::PinInstance* pin, bool collapsed_mode, bool align_to_widget);
    void draw_pin_shape(data::PinInstance* pin, bool collapsed_mode, bool align_to_widget);
    void draw_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_int(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_uint(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_float(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_str_ml(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_combo(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_separator(float offset, bool& opened, bool& temp_open);

    void handle_widget_active_state(data::PinInstance* pin, const primitive* backup = nullptr);
    bool handle_node_resize();
    bool handle_pin_renaming(data::PinInstance* curr_pin);
    void handle_deleted();

private:
    data::Workflow* m_wf = nullptr;
    ax::NodeEditor::EditorContext* m_context = nullptr;

    ImRect m_view_rect;
    bool m_view_set = true;
    float m_zoom;
    bool m_zoom_set = true;

    data::NodeInstance* m_active_node = nullptr;
    const char* m_active_tt = nullptr;

    ImU32 m_defclr_node_bg;
    ImU32 m_defclr_node_header_bg;
    ImU32 m_clr_node_header_text;
    const ImU32 m_clr_node_pin_text;
    const float m_mode_alpha = 0.4f;

    data::PinInstance* m_renaming_pin = nullptr;
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

    primitive m_old_pin_val_pr;
    int32_t m_old_pin_val_ci;
    data::PinInstance* m_editing = nullptr;
    data::PinInstance* m_was_edited = nullptr;

    std::set<void*> m_pending_delete;
    blob::Blob m_state;
    bool m_navigate_to_content = false;

#ifdef _DEBUG
    std::set<void*> m_ids_imgui;
    std::set<void*> m_ids_nodelib;
#endif
};


} // namespace ui
