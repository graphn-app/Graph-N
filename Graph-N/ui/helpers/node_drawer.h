
#pragma once

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

    using GetAllNodesCallback = std::function<const std::vector<data::NodeInstance*>&()>;
    using DeleteNodeCallback = std::function<void(data::NodeInstance*)>;
    using DeleteGroupCallback = std::function<void(data::NodeInstance*)>;
    using DeleteLinkCallback = std::function<void(data::Link*)>;
    using CopyNodeCallback = std::function<void(data::NodeInstance*, const ImRect&)>;
    using CopyGroupCallback = std::function<void(const ImRect&)>;
    using CreateGroupCallback = std::function<void(const ImRect&)>;

    struct Callbacks
    {
        GetAllNodesCallback get_all_nodes = nullptr;
        DeleteNodeCallback delete_node = nullptr;
        DeleteGroupCallback delete_group = nullptr;
        DeleteLinkCallback delete_link = nullptr;
        CopyNodeCallback copy_node = nullptr;
        CopyGroupCallback copy_group = nullptr;
        CreateGroupCallback create_group = nullptr;
    };

    void init(wid wf_id, const Callbacks& callbacks);
    void free();

    void pre_draw_all(const char* id, size_t nodes_count);
    void post_draw_all();

    void pre_draw_node(data::NodeInstance* node);
    void draw_node(data::NodeInstance* node, float node_w, size_t node_idx);
    void draw_group(data::NodeInstance* node, size_t node_idx);
    void post_draw_node(data::NodeInstance* node);

    void draw_links(const std::vector<std::unique_ptr<data::Link>>& links);
    bool query_links();

    void locate_node(data::NodeInstance* node);

    inline ImGuiExNe::DeferredAction* get_deffered_action() const { return m_deffered_action.get(); }
    data::PinInstance* get_ml_widget_active() const;

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
    bool handle_context_menu();

    bool show_context_menu();
    bool show_canvas_menu();
    bool show_node_menu();
    bool show_group_menu();
    bool show_multinode_menu();
    bool show_multigroup_menu();
    bool show_multiobj_menu();
    bool show_pin_menu();
    bool show_link_menu();
    bool show_multilink_menu();
    bool show_colors_menu(bool& close, bool& reset, bool use_custom, bool show_alpha, const ImVec4& init);

    std::set<data::NodeInstance*> get_nodes_in_group(data::NodeInstance* group);

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
    float m_color_buff[4] = { -1.0f };

    Callbacks m_callbacks;

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

    const ImGuiID m_context_menuid;
    bool m_handle_context_menu = false;

    enum class MenuType { _none, Back, Node, Group, Multinode, Multigroup, Multiobj, Pin, Link, Multilink };

    struct NodeAction final
    {
        std::set<data::NodeInstance*> nodes;
        std::map<data::NodeInstance*, std::set<data::NodeInstance*>> groups;
        std::set<data::Link*> links;
        std::set<data::PinInstance*> pins;
        ImVec2 click_pos;
    };
    MenuType m_context_menu_type = MenuType::_none;
    NodeAction m_menu_objs;
    NodeAction m_copied_objs;

#ifdef _DEBUG
    std::set<void*> m_ids_imgui;
    std::set<void*> m_ids_nodelib;
#endif
};


} // namespace ui
