
#pragma once
#include "core/base/structs.h"
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
    NodeDrawer(data::Workflow* wf) noexcept;
    ~NodeDrawer() noexcept;

    NO_COPY_MOVE(NodeDrawer);

    void init(blob::Blob&& state);
    void free() noexcept;

    void pre_draw_all(const char* id, size_t nodes_count);
    void post_draw_all(
        const std::function<bool()>& init_menu_handler,
        const std::function<bool()>& show_menu_handler,
        const std::function<bool()>& move_handler,
        const std::function<bool()>& hotkey_handler);

    void pre_draw_node(data::NodeInstance* node, bool execution_failed);
    void draw_node(data::NodeInstance* node, float node_w, size_t node_idx, const std::set<std::string>* failed_pins);
    void draw_group(data::NodeInstance* node, size_t node_idx);
    void post_draw_node(data::NodeInstance* node, bool execution_failed) noexcept;

    void draw_links(const std::vector<std::unique_ptr<data::Link>>& links);
    void query_new_item(const std::function<void(data::PinInstance*)>& unlink_handler, const std::function<void(data::PinInstance*, const ImVec2&)>& new_node_handler);

    void locate_node(data::NodeInstance* node) noexcept;
    void locate_selected() noexcept;

    ImGuiExNe::DeferredAction* get_deffered_action() const noexcept { return m_deffered_action.get(); }
    data::PinInstance* get_ml_widget_active() const noexcept;

    ImVec2 get_mouse_pos(const ImVec2* convert_pos = nullptr) const noexcept;
    bool get_bg_menu_active(bool& rclick, bool& dbl_click) const noexcept;

    data::PinInstance* get_hovered_pin() const;
    data::NodeInstance* get_hovered_node() const;
    data::Link* get_hovered_link() const;
    
    std::set<data::NodeInstance*> get_selected_nodes() const;
    bool is_node_selected(const EdType& node) const noexcept;
    void select_node(const EdType& node, bool append_to_selection) const noexcept;

    std::set<data::Link*> get_selected_links() const;
    bool is_link_selected(const EdType& link) const noexcept;
    void select_link(const EdType& link, bool append_to_selection) const noexcept;

    void clear_selection() noexcept;

    std::set<data::NodeInstance*> get_nodes_in_group(const data::NodeInstance* group, const std::vector<std::unique_ptr<data::NodeInstance>>& all_nodes) const;
    ImRect calc_group_rect(const ImRect& node_rc) const noexcept;
    ImRect fit_group_rect(const data::NodeInstance* group, const std::set<data::NodeInstance*>& nodes) const noexcept;

    void pin_node(const EdType& node, bool pin) const noexcept;
    void rename_node(data::NodeInstance* node) noexcept;
    void rename_pin(data::PinInstance* pin) noexcept;
    void handle_node_collapsing(const data::NodeInstance* node);

    void delete_node(data::NodeInstance* node);
    void delete_link(data::Link* link);

    ImU32 get_default_node_color() const noexcept { return m_defclr_node_bg; }

    float get_zoom() const noexcept;
    void set_zoom(float zoom) noexcept;
    const ImRect& get_viewport_rect() const noexcept;
    void set_viewport_rect(const ImRect& rect) noexcept;

    ImVec2 get_visible_pos(std::optional<ImVec2> add_offset) const noexcept;

    const blob::Blob& get_state() const noexcept { return m_state; }

    data::NodeInstance* get_node_from_ed(const EdType& ed) const;

private:
    void draw_node_header();
    void draw_collapsed_pins(bool input);
    void draw_input_pin(data::PinInstance* pin, bool failed_pin);
    void draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_output_pin(data::PinInstance* pin);
    void draw_pin(data::PinInstance* pin, bool collapsed_mode, bool align_to_widget, bool failed_pin);
    void draw_pin_shape(data::PinInstance* pin, bool collapsed_mode, bool align_to_widget, bool failed_pin);
    void draw_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_int(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_float(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_number(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_str_ml(data::PinInstance* pin, float widget_name_width, float widget_value_width) noexcept;
    void draw_widget_combo(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width);
    void draw_widget_separator(float offset, bool& opened, bool& temp_open) noexcept;

    void handle_widget_active_state(data::PinInstance* pin, const Primitive* backup = nullptr);
    bool handle_node_resize() noexcept;
    bool handle_pin_renaming(const data::PinInstance* curr_pin);
    void handle_deleted() noexcept;

private:
    data::Workflow* m_wf = nullptr;
    ax::NodeEditor::EditorContext* m_context = nullptr;

    ImRect m_view_rect;
    bool m_view_set = true;
    float m_zoom = 1.0f;
    bool m_zoom_set = true;

    data::NodeInstance* m_active_node = nullptr;
    const char* m_active_tt = nullptr;

    ImU32 m_defclr_node_bg = 0;
    ImU32 m_defclr_node_header_bg = 0;
    ImU32 m_clr_node_header_text = 0;
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

    Primitive m_old_pin_val_pr;
    int32_t m_old_pin_val_ci = 0;
    data::PinInstance* m_editing = nullptr;
    data::PinInstance* m_was_edited = nullptr;

    blob::Blob m_state;
    bool m_navigate_to_content = false;

    // sorted vector is still faster than a map; 1000 objects will fit into the L1/L2 cache
    std::vector<std::pair<uint64_t, data::NodeInstance*>> m_ed_objects_nodes;
    std::vector<std::pair<uint64_t, data::PinInstance*>> m_ed_objects_pins;
    std::vector<std::pair<uint64_t, data::Link*>> m_ed_objects_links;
};


} // namespace ui
