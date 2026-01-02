
#include "ui/workflow/workflow_drawer.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "imgui_node_editor.h"

namespace ui
{

namespace ed = ax::NodeEditor;

inline constexpr float g_pin_center_x_offset = 10.0f;
inline constexpr float g_pin_radius = 5.0f;
inline constexpr float g_pin_width = g_pin_radius * 2;
inline constexpr float g_pin_offset = g_pin_width;
inline constexpr float g_node_def_width = 150.0f;

WorkflowDrawer::WorkflowDrawer()
{}

WorkflowDrawer::~WorkflowDrawer()
{
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

void WorkflowDrawer::draw()
{
    ImGuiIO& io = ImGui::GetIO();
    auto old_flags = io.ConfigFlags;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    const ImVec2 canvas_size(-FLT_MIN, -FLT_MIN);
    auto old_editor = ed::GetCurrentEditor();
    ed::SetCurrentEditor(m_context);
    ed::Begin(m_wf->get_id_str().c_str(), canvas_size);

    auto& nodes = m_wf->get_nodes();
    auto& links = m_wf->get_links();

    for (const auto& node_inst : nodes)
    {
        if (!node_inst->is_valid())
        {
            LogError(ExcepCategory::logic, "Null node");
            continue;
        }

        auto node = node_inst->get_object();
        if (!node->node_width_ready)
            calc_node_width(node_inst.get());

        ed::BeginNode(node_inst->get_graph_id());
        ImGui::Dummy(ImVec2(node->node_width, 0.0f));

        ImGui::BeginGroup();
        draw_node(node_inst.get());
        ImGui::EndGroup();

        const auto& input = node_inst->get_input();
        const auto& output = node_inst->get_output();

        ImGui::BeginGroup();

        for (auto pin : input)
        {
            if (nullptr == pin || !pin->is_valid())
            {
                LogError(ExcepCategory::logic, "Null pin");
            }
            else if (!pin->get_state()->advanced && !pin->get_object()->is_widget())
            {
                ed::BeginPin(pin->get_graph_id(), ed::PinKind::Input);
                draw_input_pin(pin);
                ed::EndPin();
            }
        }

        ImGui::EndGroup();
        ImGui::SameLine(0, g_pin_center_x_offset * 2);
        ImGui::BeginGroup();

        for (auto pin : output)
        {
            if (nullptr == pin || !pin->is_valid())
            {
                LogError(ExcepCategory::logic, "Null pin");
            }
            else
            {
                const float content_width = pin->get_object()->name_width + g_pin_width + g_pin_offset;
                const float shift = node->right_group_width - content_width;
                if (shift > 0.0f)
                {
                    //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + shift);
                    ImGui::Dummy(ImVec2(shift, 0));
                    ImGui::SameLine();
                }

                ed::BeginPin(pin->get_graph_id(), ed::PinKind::Output);
                draw_output_pin(pin);
                ed::EndPin();
            }
        }

        ImGui::EndGroup();

        ImGui::BeginGroup();
        for (auto pin : input)
        {
            if (!pin->get_state()->advanced && pin->get_object()->is_widget())
            {
                draw_input_widget(pin);
            }
        }
        ImGui::EndGroup();

        ed::EndNode();
    }

    for (const auto& link : links)
    {
        draw_link(link.get());
    }

    // ed::NavigateToContent();

    ed::End();
    ed::SetCurrentEditor(old_editor);

    io.ConfigFlags = old_flags;
}

void WorkflowDrawer::calc_node_width(const data::NodeInstance* node)
{
    float max_in = 0.0f;
    float max_out = 0.0f;

    auto& in = node->get_input();
    for (auto pin : in)
    {
        auto obj = pin->get_object();
        if (!obj->is_widget())
        {
            obj->name_width = ImGui::CalcTextSize(obj->name.c_str()).x;
            if (obj->name_width > max_in)
                max_in = obj->name_width;
        }
    }

    auto& out = node->get_output();
    for (auto pin : out)
    {
        auto obj = pin->get_object();
        obj->name_width = ImGui::CalcTextSize(obj->name.c_str()).x;
        if (obj->name_width > max_out)
            max_out = obj->name_width;
    }

    max_in += g_pin_width + g_pin_offset;
    max_out += g_pin_width + g_pin_offset;

    auto node_obj = node->get_object();
    node_obj->node_name_width = ImGui::CalcTextSize(node_obj->display_name.empty() ? node_obj->name.c_str() : node_obj->display_name.c_str()).x;
    node_obj->node_width = std::max(g_node_def_width, node_obj->node_name_width);

    float total_width = max_in + g_pin_center_x_offset * 2 + max_out;
    node_obj->node_width = std::max(total_width, node_obj->node_width);

    total_width = node_obj->node_width;
    node_obj->left_group_width = node_obj->right_group_width = total_width / 2 - g_pin_center_x_offset;

    if (max_in > node_obj->left_group_width)
    {
        node_obj->left_group_width = max_in;
        node_obj->right_group_width = total_width - max_in - g_pin_center_x_offset * 2;
    }
    else if (max_out > node_obj->right_group_width)
    {
        node_obj->right_group_width = max_out;
        node_obj->left_group_width = total_width - max_out - g_pin_center_x_offset * 2;
    }

    for (auto pin : in)
    {
        auto obj = pin->get_object();
        if (obj->is_widget())
        {
            obj->name_width = total_width - g_pin_width * 2 - g_pin_offset * 2;
        }
    }

    node_obj->node_width_ready = true;
}

void WorkflowDrawer::draw_node(const data::NodeInstance* node)
{
    auto obj = node->get_object();
  //  ImGui::SetCursorPosX(obj->node_width / 2 - obj->node_name_width / 2);
    ImGui::Dummy(ImVec2(obj->node_width / 2 - obj->node_name_width / 2, 0));
    ImGui::SameLine();
    ImGui::TextUnformatted(obj->display_name.empty() ? obj->name.c_str() : obj->display_name.c_str());
}

void WorkflowDrawer::draw_input_pin(const data::PinInstance* pin)
{
    auto obj = pin->get_object();
    draw_pin_icon(pin->get_state()->pin_color, pin->get_state()->links > 0);
    //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + g_pin_width + g_pin_offset);
    ImGui::Dummy(ImVec2(g_pin_width + g_pin_offset, 0));
    ImGui::SameLine();
    ImGui::TextUnformatted(obj->name.c_str());
}

void WorkflowDrawer::draw_output_pin(const data::PinInstance* pin)
{
    ImGui::TextUnformatted(pin->get_object()->name.c_str());
    ImGui::SameLine();
    draw_pin_icon(pin->get_state()->pin_color, pin->get_state()->links > 0);
}

void WorkflowDrawer::draw_link(const data::Link* link)
{
    ed::Link(link->get_graph_id(), link->get_input()->get_graph_id(), link->get_output()->get_graph_id(), link->get_input()->get_state()->pin_color);
}

void WorkflowDrawer::draw_pin_icon(ImU32 color, bool connected)
{
    const auto drawList = ImGui::GetWindowDrawList();
    const auto cursorPos = ImGui::GetCursorScreenPos();

    const ImVec2 center = ImVec2(cursorPos.x + g_pin_radius, cursorPos.y + ImGui::GetTextLineHeight() / 2);

    if (connected)
        drawList->AddCircleFilled(center, g_pin_radius, color);
    else
        drawList->AddCircle(center, g_pin_radius, color, 0, 1.0f);
}

void WorkflowDrawer::draw_input_widget(const data::PinInstance* pin)
{
    auto obj = pin->get_object();

    //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + g_pin_radius + g_pin_radius / 2);
    ImGui::Dummy(ImVec2(g_pin_radius + g_pin_radius / 2, 0));
    ImGui::SameLine();
    ImGui::TextUnformatted(obj->name.c_str());
}

} // namespace ui
