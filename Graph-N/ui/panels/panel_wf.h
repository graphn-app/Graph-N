
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/node_drawer.h"
#include "core/listeners.h"

namespace data
{
struct Node;
class Link;
class NodeInstance;
class Workflow;
class WorkflowAction;
}

namespace ui
{

class PanelWorkflow final :
    public PanelToolbar,
    public PanelEmptyWithDescr,
    public PanelBase<PanelCommonData>,
    public IsAlive,
    public listener::ComfyUiObjectValidityListener
{
public:
    PanelWorkflow(wid workflow_id, const std::string& name, const std::string& panel_id, ImGuiID dock);
    ~PanelWorkflow() noexcept override;

    void init() override;    

    void stop_all_threads() noexcept override;
    void free() noexcept override;

    data::Workflow* get_workflow() const noexcept { return m_wf; }
    data::WorkflowAction* get_action() const noexcept { return m_action; }

    void select_new_nodes() noexcept { m_select_new_nodes = true; }
    void select_all_nodes() noexcept { m_select_all_nodes = true; }

    void restore_pos() noexcept { m_restore_pos = true; }
    void rename_panel(const std::string& new_name) { set_name(new_name); }

    void locate_node(data::NodeInstance* node) noexcept { m_locate = node; }
    void locate_selected_node() noexcept { m_locate_selected = true; }

    void set_wf_view(std::optional<ImRect> wf_rc, std::optional<float> wf_zoom);

    void display_new_node_menu() noexcept { m_display_new_node_menu = true; }

    bool can_run() const noexcept;
    void run(bool add_front);

    void set_num_of_runs(int32_t num) noexcept;

    bool get_node_selected(data::NodeInstance* node) const noexcept;
    std::set<data::NodeInstance*> get_selected_nodes() const { return m_drawer.get_selected_nodes(); }

    bool can_save() const noexcept { return m_can_save && !m_invalidation_state; }
    void save() const;

    bool can_undo() const noexcept { return *m_undo_name != 0 && !m_invalidation_state; }
    void undo() const;

    bool can_redo() const noexcept { return *m_redo_name != 0 && !m_invalidation_state; }
    void redo() const;

    void group_nodes(const std::set<data::NodeInstance*>& nodes);
    void delete_nodes(const std::set<data::NodeInstance*>& nodes);
    void copy_nodes(const std::set<data::NodeInstance*>& nodes, std::optional<ImVec2> src_point);
    void paste_copied(std::optional<ImVec2> paste_at);

    void split_reroutes(const std::set<data::NodeInstance*>& nodes);
    void merge_reroutes(const std::set<data::NodeInstance*>& nodes);
    void insert_reroute(data::Link* link, const ImVec2& pos);

private:
    DlgCode show(bool& focused) override;
    void draw_wf();
    void handle_panel_context_menu() noexcept;

    // ComfyUiObjectValidityListener
    void invalidate_comfyui_objects() noexcept override;
    void resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step step) override;

    void check_history();
    void create_toolbar();

    void on_pre_save(PanelToolbar::Element& el) noexcept;
    void on_save(PanelToolbar::Element& el);
    void on_pre_revert(PanelToolbar::Element& el) noexcept;
    void on_revert(PanelToolbar::Element& el);
    void on_pre_undo(PanelToolbar::Element& el) noexcept;
    void on_undo(PanelToolbar::Element& el);
    void on_pre_redo(PanelToolbar::Element& el) noexcept;
    void on_redo(PanelToolbar::Element& el);
    void on_pre_project_status(PanelToolbar::Element& el) noexcept;

    void on_pre_queue_status(PanelToolbar::Element& el); 
    void on_pre_queue(PanelToolbar::Element& el) noexcept;
    void on_queue(PanelToolbar::Element& el);
    void on_pre_queue_interrupt(PanelToolbar::Element& el) noexcept;
    void on_queue_interrupt(PanelToolbar::Element& el);
    void on_pre_queue_drop_all(PanelToolbar::Element& el);
    void on_queue_drop_all(PanelToolbar::Element& el);
    void on_pre_tasks_num(PanelToolbar::Element& el) noexcept;
    void on_tasks_num(PanelToolbar::Element& el);
    void on_show_dummy_node(PanelToolbar::Element& el);

    bool context_menu_handler();
    bool show_context_menu();
    bool show_canvas_menu();
    bool show_canvas_nodes_menu();
    bool show_node_menu();
    bool show_group_menu();
    bool show_multinode_menu();
    bool show_multigroup_menu();
    bool show_multiobj_menu();
    bool show_pin_menu();
    bool show_link_menu();
    bool show_multilink_menu();
    bool show_new_node_menu();
    bool show_colors_menu(bool& close, bool& reset, bool show_alpha, const ImVec4& init);
    data::Node* show_nodes_menu(const std::list<PinType>& in_pins, const std::list<PinType>& out_pins);

    void fit(std::map<data::NodeInstance*, std::set<data::NodeInstance*>>& items);

    void handle_dnd();
    bool handle_moves();

private:
    const ImGuiID m_dock;
    data::Workflow* m_wf = nullptr;
    bool m_wf_valid = true;
    data::WorkflowAction* m_action = nullptr;
    NodeDrawer m_drawer;

    bool m_restore_pos = false;
    data::NodeInstance* m_locate = nullptr;
    bool m_locate_selected = false;
    bool m_select_new_nodes = false;
    bool m_select_all_nodes = false;
    bool m_first_show = true;

    const std::string m_context_menuid;
    bool m_handle_context_menu = false;
    bool m_context_menu_nodes_need_focus = false;
    bool m_display_new_node_menu = false;
    std::string m_context_menu_nodes_filter;
    fmt::memory_buffer m_fmt_buffer;
    fmt::memory_buffer m_status_buffer;
    const std::map<iid, std::set<std::string>>* m_failed_nodes = nullptr;

    enum class MenuType { _none, Back, BackDblClick, Node, Group, Multinode, Multigroup, Multiobj, Pin, Link, Multilink, SpawnNode };
    struct MenuObjects final
    {
        std::set<data::NodeInstance*> nodes;
        std::map<data::NodeInstance*, std::set<data::NodeInstance*>> groups;
        std::set<data::Link*> links;
        std::set<data::PinInstance*> pins;
        ImVec2 click_pos;
        bool align_menu_pos = false;
    };
    MenuType m_context_menu_type = MenuType::_none;
    MenuObjects m_menu_objs;
    float m_color_buff[4] = { -1.0f };

    int32_t m_tasks_num = 1;
    std::string m_queue_num_input_id;

    bool m_can_save = false;
    bool m_can_revert = false;
    char m_history_status[64] = {};
    char m_tt_history_status[128] = {};
    char m_undo_name[64] = {};
    char m_redo_name[64] = {};
    char m_tt_revert[128] = {};

    int32_t m_curr_dummy_node = 0;
    bool m_invalidation_state = false;
};


} // namespace ui

