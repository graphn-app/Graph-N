
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

    void add_node(data::Node* node, const std::string& select = "", std::optional<ImVec2> pos = std::nullopt);

private:
    DlgCode show(bool& focused) override;
    void draw_wf();

    void on_delete_node(data::NodeInstance* node);
    void on_delete_link(data::Link* link);
    void on_copy_node(data::NodeInstance* node);
    void on_create_group(const ImRect& rc);

private:
    wid m_wf = 0;
    const ImGuiID m_dock;
    bool m_restore_pos = false;
    bool m_active = false;
    std::vector<data::NodeInstance*> m_nodes;
    NodeDrawer m_drawer;
    data::NodeInstance* m_locate = nullptr;
};


} // namespace ui

