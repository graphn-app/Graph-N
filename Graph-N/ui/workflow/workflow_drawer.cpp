
#include "ui/workflow/workflow_drawer.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/settings_manager.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "imgui_node_editor.h"

namespace ui
{

namespace ed = ax::NodeEditor;

WorkflowDrawer::WorkflowDrawer()
    : m_clr_node_header_bg(g::pOptsManager->color_load(AppColor::NodeHeaderBg))
    , m_clr_node_header_text(g::pOptsManager->color_load(AppColor::NodeHeaderText))
    , m_clr_node_body_bg(g::pOptsManager->color_load(AppColor::NodeBodyBg))
    , m_clr_node_pin_text(g::pOptsManager->color_load(AppColor::NodePinText))
    , m_clr_node_border(g::pOptsManager->color_load(AppColor::NodeBorder))
{}

WorkflowDrawer::~WorkflowDrawer()
{
    LogF;

    if (m_context != nullptr)
        ed::DestroyEditor(m_context);
}

void WorkflowDrawer::init(data::Workflow* wf)
{
    LogF;

    ed::Config config;
    config.SettingsFile = nullptr;
    m_context = ed::CreateEditor(&config);
    
    m_wf = wf;
}

void WorkflowDrawer::draw(float /*max_w*/)
{
    /*
    ImGuiIO& io = ImGui::GetIO();
    ImGuiConfigFlags old_flags = io.ConfigFlags;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    ax::NodeEditor::EditorContext* old_context = ed::GetCurrentEditor();
    ed::SetCurrentEditor(m_context);
    ed::Begin(m_wf->get_id_str().c_str(), ImVec2(-FLT_MIN, -FLT_MIN));

    auto& nodes = m_wf->get_nodes();
    const float section_spacing = 6.0f;

    for (const auto& node_inst : nodes)
    {
        if (!node_inst->is_valid())
        {
            LogError(ExcepCategory::Logic, "Null node");
            continue;
        }

        auto node = node_inst->get_object();
        if (!node->node_width_ready)
            node_inst->calc_node_width();

        node_inst->set_node_width(max_w, false);
        ImGuiEx::PushID(node_inst->get_graph_id());

        // --- NODE FRAME ---

        ed::BeginNode(node_inst->get_graph_id());
        ImGui::Dummy(ImVec2(node->node_width, 0.0f));

        // --- CULLING ---

        const ImVec2 node_size = ed::GetNodeSize(node_inst->get_graph_id());
        if (node_size.x > 0 && node_size.y > 0)
        {
            if (!ImGui::IsRectVisible(node_size))
            {
                ImGui::Dummy(node_size);
                ed::EndNode();
                ImGui::PopID(); // node_inst id
                continue;
            }
        }


        // --- HEADER ---

        ImGui::BeginGroup();
        draw_node(node_inst.get());
        ImGui::EndGroup();

        ImGui::Dummy(ImVec2(0.0f, section_spacing));

        // --- PINS - INPUT ---

        ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_pin_text);
        ImGui::BeginGroup();

        const auto& input = node_inst->m_input;
        for (auto pin : input)
        {
            if (nullptr == pin || !pin->is_valid())
            {
                LogError(ExcepCategory::Logic, "Null pin");
            }
            else if (!pin->m_state->advanced && !pin->get_object()->is_widget())
            {
                ImGuiEx::PushID(pin->get_graph_id());
                ed::BeginPin(pin->get_graph_id(), ed::PinKind::Input);
                draw_input_pin(pin);
                ed::EndPin();
                ImGui::PopID(); // pin id
            }
        }
        ImGui::EndGroup();

        // --- SPACER ---

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(mngr::SettingsManager::wf_get_Pin_center_x_offset() * 2, 0.0f));
        ImGui::SameLine();

        // --- PINS - OUTPUT ---

        ImGui::BeginGroup();

        ImGui::Dummy(ImVec2(node->right_group_width, 0.0f));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);

        const auto& output = node_inst->m_output;
        for (auto pin : output)
        {
            if (nullptr == pin || !pin->is_valid())
            {
                LogError(ExcepCategory::Logic, "Null pin");
            }
            else
            {
                ImGuiEx::PushID(pin->get_graph_id());

                const float start_pos = node_inst->get_node_width() - mngr::SettingsManager::wf_get_pin_indent() - mngr::SettingsManager::wf_get_Pin_width() - pin->get_object()->name_width - ImGui::GetStyle().ItemSpacing.x;
                ImGui::SetCursorPosX(start_pos);

                ed::BeginPin(pin->get_graph_id(), ed::PinKind::Output);
                draw_output_pin(pin);
                ed::EndPin();
                ImGui::PopID(); // pin id
            }
        }
        ImGui::EndGroup();

        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0.0f, section_spacing * 2));

        // --- WIDGETS ---

        ImGui::BeginGroup();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, ImGui::GetStyle().ItemSpacing.y * 2));
        for (auto pin : input)
        {
            if (!pin->m_state->advanced && pin->get_object()->is_widget())
            {
                ImGuiEx::PushID(pin->get_graph_id());
                draw_input_widget(pin, node_inst->get_right_group_width(), node_inst->get_node_width(), false);
                ImGui::PopID(); // widget id

                if (pin->get_object()->control_after_generate)
                {
                    ImGuiEx::PushID(pin->get_graph_id());
                    draw_input_widget(pin, node_inst->get_right_group_width(), node_inst->get_node_width(), true);
                    ImGui::PopID(); // widget id
                }
            }
        }
        ImGui::PopStyleVar();
        ImGui::EndGroup();
        ImGui::Dummy(ImVec2(0, 30.0f));

        // --- FINALIZE ---

        ed::EndNode();
        ImGui::PopID(); // node_inst id
    }

    auto& links = m_wf->get_links();
    for (const auto& link : links)
    {
        draw_link(link.get());
    }

    ed::End();
    ed::SetCurrentEditor(old_context);
    ImGui::GetIO().ConfigFlags = old_flags;
    */
}

void WorkflowDrawer::draw_link(const data::Link* link)
{
    ed::Link(link->get_graph_id(), link->get_input()->get_graph_id(), link->get_output()->get_graph_id(), link->get_input()->m_state->pin_color);
}

} // namespace ui
