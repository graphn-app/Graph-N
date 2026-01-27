
#pragma once
#include "ui/panels/panel_base.h"
#include "ui/helpers/node_drawer.h"
#include "imgui_node_editor.h"

// TODO: remove
//
//#include "imgui_impl_win32.h"
//#include "imgui_impl_dx11.h"
//#include <d3d11.h>
//
//namespace ed = ax::NodeEditor;
//
//struct RealNode {
//    int ID;
//    float X, Y;
//
//    // Node State
//    int Seed = 0;
//    int Steps = 20;
//    float Cfg = 8.0f;
//
//    // Combo selections
//    int SamplerIdx = 0;
//    int SchedulerIdx = 0;
//
//    // Prompts
//    char PositivePrompt[512];
//    char NegativePrompt[512];
//};
//
//struct RealLink {
//    int ID;
//    int StartPinID;
//    int EndPinID;
//};
//
//class ComplexStressTest {
//public:
//    void Initialize();
//    void Render(ed::EditorContext* context, ID3D11ShaderResourceView* texture);
//
//private:
//    std::vector<RealNode> m_Nodes;
//    std::vector<RealLink> m_Links;
//    bool m_FirstFrame = true;
//    int m_ActiveEditNodeID = -1;
//    std::map<int, float> m_NodeHeights;
//
//    // Dummy data for combos
//    const char* m_Samplers[4] = { "euler", "euler_a", "heun", "dpm_2" };
//    const char* m_Schedulers[3] = { "normal", "karras", "exponential" };
//};

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

    // TODO: remove
    //ComplexStressTest g_StressTest;
};


} // namespace ui

