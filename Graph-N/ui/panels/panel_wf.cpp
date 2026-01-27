
#include "ui/panels/panel_wf.h"
#include "ui/graph-n_flow.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "imgui_node_editor.h"
#include "ui/helpers/imgui_ex_ne.h"


// TODO: remove
//
//#include "imgui_impl_win32.h"
//#include "imgui_impl_dx11.h"
//#include <d3d11.h>
//
//void ComplexStressTest::Initialize() {
//    m_Nodes.reserve(1000);
//    // Grid layout for 1000 nodes
//    const int cols = 35; 
//    const float spacingX = 450.0f; // Wider nodes for realistic prompts
//    const float spacingY = 600.0f; // Taller nodes
//
//    for (int i = 0; i < 1000; i++) {
//        RealNode node;
//        node.ID = i + 1;
//
//        int col = i % cols;
//        int row = i / cols;
//        node.X = col * spacingX;
//        node.Y = row * spacingY;
//
//        // Fill with dummy text
//        snprintf(node.PositivePrompt, sizeof(node.PositivePrompt), 
//            "masterpiece, best quality, ultra detailed, node_%d, 8k resolution, cinematic lighting", i);
//        snprintf(node.NegativePrompt, sizeof(node.NegativePrompt), 
//            "lowres, bad anatomy, bad hands, text, error, missing fingers, extra digit, fewer digits, cropped");
//
//        m_Nodes.push_back(node);
//
//        // Simple chaining logic for links
//        if (i > 0) {
//            RealLink link;
//            link.ID = 10000 + i;
//            link.StartPinID = (i - 1) * 10 + 1;
//            link.EndPinID = i * 10 + 2;
//            m_Links.push_back(link);
//        }
//    }
//}

/*
void ComplexStressTest::Render(ed::EditorContext* context, ID3D11ShaderResourceView* texture) {
    ed::SetCurrentEditor(context);
    ed::Begin("Graph-N Final Fix");

    // Инфо-панель
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "AABB CULLING + LAZY EDIT | Vtx: %d", 
        (int)ImGui::GetIO().MetricsRenderVertices);

    if (m_FirstFrame) {
        for (const auto& node : m_Nodes) {
            ed::SetNodePosition((ed::NodeId)node.ID, ImVec2(node.X, node.Y));
        }
        m_FirstFrame = false;
    }

    // --- 1. ПРАВИЛЬНЫЙ CULLING (AABB Intersection) ---
    // Берем экранные координаты вьюпорта
    ImVec2 viewMin = ImGui::GetCursorScreenPos();
    ImVec2 viewMax = ImVec2(viewMin.x + ImGui::GetContentRegionAvail().x, 
        viewMin.y + ImGui::GetContentRegionAvail().y);

    // Переводим в координаты канваса
    ImVec2 cMin = ed::ScreenToCanvas(viewMin);
    ImVec2 cMax = ed::ScreenToCanvas(viewMax);

    // Добавляем padding (запас), чтобы ноды не "моргали" на краях
    float padding = 200.0f;
    cMin.x -= padding; cMin.y -= padding;
    cMax.x += padding; cMax.y += padding;

    for (auto& node : m_Nodes) {
        ed::BeginNode(node.ID);
        ImGui::PushID(node.ID); // Изолируем ID контролов внутри ноды

        ImGui::Text("<b>KSampler %d</b>", node.ID);

        // Пины (рисуем всегда для связей)
        ed::BeginPin(node.ID * 10 + 2, ed::PinKind::Input);
        ImGui::Text("-> In");
        ed::EndPin();
        ImGui::SameLine();
        ed::BeginPin(node.ID * 10 + 1, ed::PinKind::Output);
        ImGui::Text("Out ->");
        ed::EndPin();

        ImGui::Separator();

        // --- ПРОВЕРКА ВИДИМОСТИ ---
        ImVec2 pos = ed::GetNodePosition(node.ID);
        ImVec2 size = ed::GetNodeSize(node.ID);
        if (size.x == 0) size = ImVec2(256, 500); // Фоллбэк для первого кадра

                                                  // Исправленная формула: ПЕРЕСЕЧЕНИЕ ПРЯМОУГОЛЬНИКОВ
                                                  // Нода видна, если её правый край > левого края экрана И левый край < правого края экрана (и так же по Y)
        bool isVisible = (pos.x < cMax.x) && (pos.x + size.x > cMin.x) &&
            (pos.y < cMax.y) && (pos.y + size.y > cMin.y);

        if (isVisible) {
            // Убрали BeginGroup, чтобы не ломать координаты

            // 1. Картинка
            if (texture) ImGui::Image((ImTextureID)texture, ImVec2(256, 256));
            else ImGui::Dummy(ImVec2(256, 256));

            ImGui::PushItemWidth(256);

            // 2. Инпуты
            ImGui::InputInt("Seed", &node.Seed);
            // Тултип
            if (ImGui::IsItemHovered()) {
                ed::Suspend(); // Выходим из контекста нод, чтобы тултип был поверх всего
                ImGui::SetTooltip("Seed value: %d", node.Seed);
                ed::Resume();
            }

            ImGui::DragInt("Steps", &node.Steps, 1, 1, 100);
            ImGui::DragFloat("CFG", &node.Cfg, 0.1f, 1.0f, 30.0f);

            ImGui::Combo("Sampler", &node.SamplerIdx, m_Samplers, IM_ARRAYSIZE(m_Samplers));

            // 3. LAZY EDITING ДЛЯ ТЕКСТА (Решение проблемы "летания")
            // Positive Prompt
            ImGui::Text("Positive:");
            if (m_ActiveEditNodeID == node.ID) {
                // РЕЖИМ РЕДАКТИРОВАНИЯ: Рисуем тяжелый InputTextMultiline
                // Ctrl+Enter чтобы сохранить и выйти (или клик мимо)
                if (ImGui::InputTextMultiline("##pos", node.PositivePrompt, sizeof(node.PositivePrompt), 
                    ImVec2(256, 60), ImGuiInputTextFlags_CtrlEnterForNewLine)) 
                {
                    // Если нажали Enter или потеряли фокус - можно закрывать, 
                    // но ImGui сам обрабатывает фокус неплохо.
                }

                // Если кликнули мимо виджета - выходим из режима редактирования
                if (ImGui::IsItemDeactivated()) {
                    m_ActiveEditNodeID = -1;
                }
            } else {
                // РЕЖИМ ПРОСМОТРА: Рисуем легкий TextWrapped
                // Выглядит как Input, но это просто геометрия. Не летает. Не тормозит.
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRect(p, ImVec2(p.x + 256, p.y + 60), IM_COL32(100, 100, 100, 255)); // Рамка
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 256);
                ImGui::TextWrapped("%s", node.PositivePrompt[0] ? node.PositivePrompt : "Empty prompt...");
                ImGui::PopTextWrapPos();

                // Невидимая кнопка поверх текста, чтобы поймать клик
                ImGui::SetCursorScreenPos(p);
                if (ImGui::InvisibleButton("##pos_btn", ImVec2(256, 60))) {
                    m_ActiveEditNodeID = node.ID; // Включаем режим редактирования для этой ноды
                }
            }

            ImGui::Dummy(ImVec2(0, 5)); // Spacing

                                        // Negative Prompt (Аналогично)
            ImGui::Text("Negative:");
            ImVec2 p2 = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRect(p2, ImVec2(p2.x + 256, p2.y + 40), IM_COL32(100, 100, 100, 255));
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 256);
            ImGui::TextWrapped("%s", node.NegativePrompt[0] ? node.NegativePrompt : "Empty negative...");
            ImGui::PopTextWrapPos();
            // В этом примере Negative пока не редактируемый для краткости, 
            // но логика такая же как выше.
            ImGui::Dummy(ImVec2(256, 40)); 

            ImGui::PopItemWidth();
        } 
        else {
            // DUMMY (Заглушка правильного размера)
            // Важно сохранять высоту, чтобы линки не "плясали"
            ImGui::Dummy(ImVec2(256, 500)); 
        }

        ImGui::PopID(); // END PushID
        ed::EndNode();
    }

    for (const auto& link : m_Links) {
        ed::Link(link.ID, link.StartPinID, link.EndPinID);
    }

    ed::End();
}
*/
/*
void ComplexStressTest::Render(ed::EditorContext* context, ID3D11ShaderResourceView* texture) {
    ed::SetCurrentEditor(context);
    ed::Begin("Graph-N Stabilized");

    // Debug overlay
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "STABLE CULLING | Vtx: %d", 
        (int)ImGui::GetIO().MetricsRenderVertices);

    if (m_FirstFrame) {
        for (const auto& node : m_Nodes) {
            ed::SetNodePosition((ed::NodeId)node.ID, ImVec2(node.X, node.Y));
        }
        m_FirstFrame = false;
    }

    // --- CULLING PREP ---
    ImVec2 viewMin = ImGui::GetCursorScreenPos();
    ImVec2 viewMax = ImVec2(viewMin.x + ImGui::GetContentRegionAvail().x, 
        viewMin.y + ImGui::GetContentRegionAvail().y);

    // Convert to Canvas Space
    ImVec2 cMin = ed::ScreenToCanvas(viewMin);
    ImVec2 cMax = ed::ScreenToCanvas(viewMax);

    // Huge padding to prevent popping at edges
    const float padding = 500.0f; 
    cMin.x -= padding; cMin.y -= padding;
    cMax.x += padding; cMax.y += padding;

    for (auto& node : m_Nodes) {
        ed::BeginNode(node.ID);
        ImGui::PushID(node.ID);

        // Header (Always visible)
        ImGui::Text("KSampler %d", node.ID);

        // Pins (Always visible for links)
        ed::BeginPin(node.ID * 10 + 2, ed::PinKind::Input);
        ImGui::Text("-> In");
        ed::EndPin();
        ImGui::SameLine();
        ed::BeginPin(node.ID * 10 + 1, ed::PinKind::Output);
        ImGui::Text("Out ->");
        ed::EndPin();

        ImGui::Separator();

        // --- STABLE CULLING CHECK ---
        ImVec2 pos = ed::GetNodePosition(node.ID);
        ImVec2 size = ed::GetNodeSize(node.ID);

        // Check intersection
        bool isVisible = (pos.x < cMax.x) && (pos.x + size.x > cMin.x) &&
            (pos.y < cMax.y) && (pos.y + size.y > cMin.y);

        // Retrieve cached height or default if never drawn
        float currentHeight = m_NodeHeights.count(node.ID) ? m_NodeHeights[node.ID] : 500.0f;

        if (isVisible) {
            ImGui::BeginGroup(); // Start measuring content

                                 // 1. Image
            if (texture) ImGui::Image((ImTextureID)texture, ImVec2(256, 256));
            else ImGui::Dummy(ImVec2(256, 256));

            ImGui::PushItemWidth(256);

            // 2. Controls
            ImGui::InputInt("Seed", &node.Seed);
            // Tooltip without Suspend (try native first, it's safer for stability)
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Seed: %d", node.Seed);
            }

            ImGui::DragInt("Steps", &node.Steps, 1, 1, 100);
            ImGui::DragFloat("CFG", &node.Cfg, 0.1f, 1.0f, 30.0f);

            // Standard Combo (Safer than custom popups for now)
            ImGui::Combo("Sampler", &node.SamplerIdx, m_Samplers, IM_ARRAYSIZE(m_Samplers));

            // 3. EDIT LOGIC (Text)
            ImGui::Text("Positive Prompt:");

            if (m_ActiveEditNodeID == node.ID) {
                // EDIT MODE
                // Ctrl+Enter to save
                if (ImGui::InputTextMultiline("##pos", node.PositivePrompt, sizeof(node.PositivePrompt), 
                    ImVec2(256, 100), ImGuiInputTextFlags_CtrlEnterForNewLine)) 
                {
                    // Logic on change if needed
                }

                // Click outside to stop editing
                if (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
                    m_ActiveEditNodeID = -1;
                }
            } 
            else {
                // VIEW MODE (Read-only TextWrapped)
                // This never "flies away" because it's just geometry
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 256);
                ImGui::TextWrapped("%s", node.PositivePrompt[0] ? node.PositivePrompt : "Empty...");
                ImGui::PopTextWrapPos();

                // Click detection on the text block itself
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                    m_ActiveEditNodeID = node.ID;
                }

                // Visual hint that it's editable
                if (ImGui::IsItemHovered()) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                }
            }

            ImGui::PopItemWidth();
            ImGui::EndGroup();

            // SAVE HEIGHT for next frame's culling/dummy
            // This is the magic fix for "Jumping/Exploding" nodes
            float h = ImGui::GetItemRectSize().y;
            if (h > 10.0f) { // Simple filter to avoid zero-height bugs
                m_NodeHeights[node.ID] = h;
            }

        } else {
            // NOT VISIBLE: Use exact cached height
            // This keeps the node rigid in the layout solver
            ImGui::Dummy(ImVec2(256, currentHeight));
        }

        ImGui::PopID();
        ed::EndNode();
    }

    for (const auto& link : m_Links) {
        ed::Link(link.ID, link.StartPinID, link.EndPinID);
    }

    ed::End();
}
*/
namespace ui
{

namespace ed = ax::NodeEditor;

PanelWorkflow::PanelWorkflow(wid workflow_id, const std::string& name, const std::string& panel_id, ImGuiID dock)
    : PanelBase(DlgObj::PanelWf, (ICON_LC_WORKFLOW " " + name).c_str(), ICON_LC_WORKFLOW, panel_id.c_str(), true)
    , m_wf(workflow_id)
    , m_dock(dock)
{
}

PanelWorkflow::~PanelWorkflow()
{
    LogF;

    if (m_context != nullptr)
        ed::DestroyEditor(m_context);
}

void PanelWorkflow::stop_all_threads()
{
    LogF;
}

void PanelWorkflow::free()
{
    LogF;
    m_wf = 0;

    if (m_context != nullptr)
        ed::DestroyEditor(m_context);
    m_context = nullptr;
}

void PanelWorkflow::init()
{
    LogF;
    PanelBase::init();

    m_nodes.reserve(100);

    ed::Config config;
    config.SettingsFile = nullptr;
    m_context = ed::CreateEditor(&config);

    // TODO: remove
    //g_StressTest.Initialize();
}

void PanelWorkflow::restore_pos()
{
    m_restore_pos = true;
}

void PanelWorkflow::rename_panel(const std::string& new_name)
{
    set_name(new_name);
}

DlgCode PanelWorkflow::show(bool& focused)
{
    bool opened = true;
    ImGui::SetNextWindowDockID(m_dock, m_restore_pos ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    m_restore_pos = false;
    if (begin_panel(opened, focused, false, m_active) && opened)
    {
        if (m_nodes.empty())
        {
            show_empty_panel_descr(
                "",
                {},
                {
                    "Empty Canvas.",
                    "Press Space, Right Click or load from " ICON_LC_BOOK_OPEN " Nodes Library to add the first node.",
                    "Drag and drop an image or .json file here to load a workflow."
                }
                );
        }
        else
            draw_wf();
    }

    return end_panel(opened);
}

void PanelWorkflow::add_node(data::Node* node, std::optional<ImVec2> pos)
{
    auto wf = g::pWfsManager->access(m_wf);
    auto node_inst = wf->add_node(node);
    node_inst->calc_node_width();

    if (pos.has_value())
        node_inst->move(pos.value());
    else
        node_inst->move(ImVec2(100, 100));
    m_nodes.push_back(node_inst);
}

void PanelWorkflow::draw_wf()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiConfigFlags old_flags = io.ConfigFlags;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    ed::SetCurrentEditor(m_context);
    ed::Begin(get_panel_id(), ImVec2(0, 0));

    for (auto node : m_nodes)
    {
        const bool deferred_node_active = ImGuiExNe::GetDeferredID() == node;
        if (deferred_node_active)
        {
            if (!ed::IsNodeSelected(cast::to_ptr(node)))
                ed::SelectNode(cast::to_ptr(node), true);

 //           const ImVec4 sel_color = ed::GetStyle().Colors[ed::StyleColor_SelNodeBorder];
 //           ed::PushStyleColor(ed::StyleColor_NodeBorder, sel_color);
  //          ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, ed::GetStyle().SelectedNodeBorderWidth); 
        }
        else if (ImGuiExNe::GetDeferredID() != nullptr)
        {
            ed::DeselectNode(cast::to_ptr(node));
        }

        ImGui::PushID(node);
        ed::BeginNode(cast::to_ptr(node));
        m_drawer.draw(node, node->get_object()->w_fit);
        ed::EndNode();
        ImGui::PopID();
    }

    ed::End();
    ed::SetCurrentEditor(nullptr);

    m_drawer.draw_deferred();
    ImGui::GetIO().ConfigFlags = old_flags;
}

} // namespace ui
