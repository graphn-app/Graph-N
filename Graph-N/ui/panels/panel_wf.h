
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/helpers/node_drawer.h"

namespace ax::NodeEditor
{
struct EditorContext;
}

namespace data
{
struct Node;
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
    inline void set_active(bool active) { m_active = active; }

    void add_node(data::Node* node, std::optional<ImVec2> pos);

private:
    DlgCode show(bool& focused) override;
    void draw_wf();

private:
    wid m_wf = 0;
    const ImGuiID m_dock;
    bool m_restore_pos = false;
    bool m_active = false;
    std::vector<data::NodeInstance*> m_nodes;
    NodeDrawer m_drawer;
    ax::NodeEditor::EditorContext* m_context = nullptr;
};


} // namespace ui
