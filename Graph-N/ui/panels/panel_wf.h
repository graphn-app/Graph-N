
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/helpers/node_drawer.h"

namespace data
{
struct Node;
class Link;
class NodeInstance;
}

namespace ui
{

class PanelWorkflow final : 
    public PanelEmptyWithDescr,
    public PanelBase<PanelCommonData>
{
public:
    PanelWorkflow(wid workflow_id, const std::string& name, const std::string& panel_id, ImGuiID dock);
    ~PanelWorkflow() override;

    void init() override;
    void stop_all_threads() override;
    void free() override;

    void restore_pos();
    void rename_panel(const std::string& new_name);
    inline wid get_workflow() const { return m_wf; }
    inline void set_active(bool active)
    {
        if (active && m_active != active)
            set_focused();
        m_active = active;
    }
    inline void locale_node(data::NodeInstance* node) { m_locate = node; }

    data::NodeInstance* add_node(data::Node* node, const std::string& select, std::optional<ImRect> pos);
    data::NodeInstance* add_group(std::optional<ImRect> size);

private:
    DlgCode show(bool& focused) override;
    void draw_wf();

    bool context_menu_handler();
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
    data::Node* show_nodes_menu(const std::string& in_pin_filter, const std::string& out_pin_filter);

    void delete_node(data::NodeInstance* node);
    void delete_group(data::NodeInstance* group);
    void delete_link(data::Link* link);
    //void copy_node(data::NodeInstance* node, const ImRect& rc); // TODO
    //void copy_group(const ImRect& rc); // TODO
    void create_group(const ImRect& rc);
    void fit(std::map<data::NodeInstance*, std::set<data::NodeInstance*>>& items);

private:
    wid m_wf = 0;
    const ImGuiID m_dock;
    bool m_restore_pos = false;
    bool m_active = false;
    std::vector<data::NodeInstance*> m_nodes;
    NodeDrawer m_drawer;
    data::NodeInstance* m_locate = nullptr;

    const std::string m_context_menuid;
    bool m_handle_context_menu = false;
    bool m_context_menu_nodes_need_focus = false;
    std::string m_context_menu_nodes_filter;
    fmt::memory_buffer m_fmt_buffer;

    enum class MenuType { _none, Back, Node, Group, Multinode, Multigroup, Multiobj, Pin, Link, Multilink };
    struct MenuObjects final
    {
        std::set<data::NodeInstance*> nodes;
        std::map<data::NodeInstance*, std::set<data::NodeInstance*>> groups;
        std::set<data::Link*> links;
        std::set<data::PinInstance*> pins;
        ImVec2 click_pos;
    };
    MenuType m_context_menu_type = MenuType::_none;
    MenuObjects m_menu_objs;
    float m_color_buff[4] = { -1.0f };
};


} // namespace ui

