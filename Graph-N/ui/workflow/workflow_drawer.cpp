
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

inline constexpr float g_pin_center_x_offset = 10.0f;
inline constexpr float g_pin_radius = 5.0f;
inline constexpr float g_pin_indent = 4.0f;
inline constexpr float g_pin_width = g_pin_radius * 2;
inline constexpr float g_node_def_width = 150.0f;

WorkflowDrawer::WorkflowDrawer(bool ro_mode)
    : m_ro_mode(ro_mode)
    , m_clr_node_header_bg(g::pOptsManager->flow_get_color(VisibleObject::WfNodeHeaderBg))
    , m_clr_node_header_text(g::pOptsManager->flow_get_color(VisibleObject::WfNodeHeaderText))
    , m_clr_node_body_bg(g::pOptsManager->flow_get_color(VisibleObject::WfNodeBodyBg))
    , m_clr_node_widget_pin(g::pOptsManager->flow_get_color(VisibleObject::WfNodeWidgetPin))
    , m_clr_node_pin_text(g::pOptsManager->flow_get_color(VisibleObject::WfNodePinText))
    , m_clr_node_border(g::pOptsManager->flow_get_color(VisibleObject::WfNodeBorder))
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

    if (!m_ro_mode)
    {
        ed::Config config;
        config.SettingsFile = nullptr;
        m_context = ed::CreateEditor(&config);
    }
    m_wf = wf;
}
void WorkflowDrawer::draw()
{
    ax::NodeEditor::EditorContext* old_context = nullptr;
    ImGuiConfigFlags old_flags = 0;

    if (m_ro_mode)
    {
        ImGui::BeginGroup();
    }
    else
    {
        ImGuiIO& io = ImGui::GetIO();
        old_flags = io.ConfigFlags;
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

        old_context = ed::GetCurrentEditor();
        ed::SetCurrentEditor(m_context);
        ed::Begin(m_wf->get_id_str().c_str(), ImVec2(-FLT_MIN, -FLT_MIN));
    }

    auto& nodes = m_wf->get_nodes();
    const float section_spacing = 6.0f;

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

        ImGui::PushID(node_inst->get_graph_id_str().c_str());

        // --- NODE FRAME ---
        if (m_ro_mode)
        {
            const auto draw_list = ImGui::GetWindowDrawList();
            draw_list->ChannelsSplit(2);
            draw_list->ChannelsSetCurrent(1);
            ImGui::BeginGroup();
            ImGui::Dummy(ImVec2(node->node_width, g_pin_indent));
        }
        else
        {
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
        }

        // --- HEADER ---
        ImGui::BeginGroup();
        draw_node(node_inst.get());
        ImGui::EndGroup();

        ImGui::Dummy(ImVec2(0.0f, section_spacing));

        // --- PINS - INPUT ---
        const auto& input = node_inst->get_input();
        const auto& output = node_inst->get_output();

        ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_pin_text);

        ImGui::BeginGroup();
        for (auto pin : input)
        {
            if (nullptr == pin || !pin->is_valid())
            {
                LogError(ExcepCategory::logic, "Null pin");
            }
            else if (!pin->get_state()->advanced && !pin->get_object()->is_widget())
            {
                ImGui::PushID(pin->get_graph_id_str().c_str());
                if (!m_ro_mode)
                    ed::BeginPin(pin->get_graph_id(), ed::PinKind::Input);
                draw_input_pin(pin);
                if (!m_ro_mode)
                    ed::EndPin();
                ImGui::PopID(); // pin id
            }
        }
        ImGui::EndGroup();

        // --- SPACER ---
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(g_pin_center_x_offset * 2, 0.0f));
        ImGui::SameLine();

        // --- PINS - OUTPUT ---
        ImGui::BeginGroup();
        //const float group_start_x = ImGui::GetCursorPosX();

        ImGui::Dummy(ImVec2(node->right_group_width, 0.0f));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);

        for (auto pin : output)
        {
            if (nullptr == pin || !pin->is_valid())
            {
                LogError(ExcepCategory::logic, "Null pin");
            }
            else
            {
                ImGui::PushID(pin->get_graph_id_str().c_str());

                const float start_pos = node->node_width - g_pin_indent - g_pin_width - /*ImGui::GetStyle().ItemSpacing.x - */pin->get_object()->name_width;
                ImGui::SetCursorPosX(start_pos);

                //const float content_w = pin->get_object()->name_width + ImGui::GetStyle().ItemSpacing.x + g_pin_width;
                //const float shift = node->right_group_width - content_w;
                //if (m_ro_mode)
                //    shift -= g_node_indent;

                //ImGui::SetCursorPosX(group_start_x + std::max(0.0f, shift));

                if (!m_ro_mode)
                    ed::BeginPin(pin->get_graph_id(), ed::PinKind::Output);
                draw_output_pin(pin);
                if (!m_ro_mode)
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
            if (!pin->get_state()->advanced && pin->get_object()->is_widget())
            {
                ImGui::PushID(pin->get_graph_id_str().c_str());
                draw_input_widget(pin, node->node_width);
                ImGui::PopID(); // widget id
            }
        }
        ImGui::PopStyleVar();
        ImGui::EndGroup();
        ImGui::Dummy(ImVec2(0, 30.0f));

        // --- FINALIZE ---
        if (!m_ro_mode)
        {
            ed::EndNode();
        }
        else
        {
            ImGui::EndGroup();

            // Channel 0
            const ImVec2 node_min = ImGui::GetItemRectMin();
            const ImVec2 node_max = ImGui::GetItemRectMax();
            const auto draw_list = ImGui::GetWindowDrawList();

            draw_list->ChannelsSetCurrent(0);

            // Body
            draw_list->AddRectFilled(node_min, node_max, m_clr_node_body_bg, 12.0f);

            // Header
            const float header_h = ImGui::GetTextLineHeight() + 8.0f + (section_spacing / 2.0f);
            draw_list->AddRectFilled(
                node_min,
                ImVec2(node_max.x, node_min.y + header_h),
                m_clr_node_header_bg,
                12.0f,
                ImDrawFlags_RoundCornersTop
            );

            // Border
            draw_list->AddRect(node_min, node_max, m_clr_node_border, 12.0f);
            draw_list->AddLine(
                ImVec2(node_min.x, node_min.y + header_h),
                ImVec2(node_max.x, node_min.y + header_h),
                m_clr_node_border
            );

            draw_list->ChannelsMerge();
            ImGui::NewLine();
        }

        ImGui::PopID(); // node_inst id
    }

    if (!m_ro_mode)
    {
        auto& links = m_wf->get_links();
        for (const auto& link : links)
        {
            draw_link(link.get());
        }

        ed::End();
        ed::SetCurrentEditor(old_context);
        ImGui::GetIO().ConfigFlags = old_flags;
    }
    else
    {
        ImGui::EndGroup();
    }
}

void WorkflowDrawer::calc_node_width(data::NodeInstance* node)
{
    auto node_obj = node->get_object();

    float max_in = 0.0f;
    float max_out = 0.0f;
    
    const float item_spacing_x = ImGui::GetStyle().ItemSpacing.x;

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

    max_in += g_pin_width + item_spacing_x;
    max_out += g_pin_width + item_spacing_x;

    node_obj->node_name_width = ImGui::CalcTextSize(node_obj->display_name.empty() ? node_obj->name.c_str() : node_obj->display_name.c_str()).x;
    node_obj->node_width = std::max(g_node_def_width, node_obj->node_name_width);

    const float center_width = (g_pin_center_x_offset * 2) + (item_spacing_x * 2);
    float total_width = max_in + center_width + max_out;

    for (auto pin : in)
    {
        auto obj = pin->get_object();
        if (obj->is_widget())
        {
            const float value_width = ImGui::CalcTextSize("18446744073709551615").x;
            const float name_width = ImGui::CalcTextSize(obj->name.c_str()).x;
            obj->name_width = name_width + 30.0f + value_width;
            if (obj->name_width + center_width > total_width)
                total_width = obj->name_width + center_width;
        }
    }

    node_obj->node_width = std::max(total_width, node_obj->node_width);
    total_width = node_obj->node_width;

    const float available = total_width - center_width;
    node_obj->left_group_width = std::max(max_in, available / 2.0f);
    node_obj->right_group_width = total_width - node_obj->left_group_width - center_width;

    if (node_obj->right_group_width < max_out)
    {
        node_obj->right_group_width = max_out;
        node_obj->node_width = node_obj->left_group_width + center_width + node_obj->right_group_width;
    }

    if (m_ro_mode)
    {
        node_obj->node_width += 16.0f;
        node_obj->right_group_width += 8.0f;
    }

    node_obj->node_width_ready = true;
}

void WorkflowDrawer::draw_node(data::NodeInstance* node)
{
    auto obj = node->get_object();

    if (!m_ro_mode)
    {
        const float border_width = ed::GetStyle().NodeBorderWidth;
        const ImVec2 cursor_screen = ImGui::GetCursorScreenPos();
        const ImVec4 padding = ed::GetStyle().NodePadding;
        const ImVec2 p_min = ImVec2(cursor_screen.x - padding.x, cursor_screen.y - padding.y);
        const float rect_w = obj->node_width + padding.x + padding.z;
        const float header_h = ImGui::GetTextLineHeight() + padding.y;

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(p_min.x + border_width * 2, p_min.y - border_width),
            ImVec2(p_min.x + rect_w - border_width * 2, p_min.y + header_h + padding.y),
            m_clr_node_header_bg,
            ed::GetStyle().NodeRounding,
            ImDrawFlags_RoundCornersTop
        );

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(p_min.x + border_width, p_min.y + header_h + padding.y),
            ImVec2(p_min.x + rect_w - border_width * 2, p_min.y + header_h + padding.y),
            m_clr_node_border
        );
    }

    float spacer = (obj->node_width - obj->node_name_width) * 0.5f;
    if (!m_ro_mode)
        spacer -= ed::GetStyle().NodePadding.x;
    if (spacer > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spacer);

    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_header_text);
    ImGui::TextUnformatted(obj->display_name.empty() ? obj->name.c_str() : obj->display_name.c_str());
    ImGui::PopStyleColor();
}

void WorkflowDrawer::draw_input_pin(data::PinInstance* pin)
{
    if (m_ro_mode)
        ImGui::Indent(g_pin_indent);

    auto obj = pin->get_object();
    draw_pin_icon(pin->get_state()->pin_color, !m_ro_mode && pin->get_state()->links > 0);
    ImGui::Dummy(ImVec2(g_pin_width, 0));
    ImGui::SameLine();
    ImGui::TextUnformatted(obj->name.c_str());

    if (m_ro_mode)
        ImGui::Unindent(g_pin_indent);
}

void WorkflowDrawer::draw_output_pin(data::PinInstance* pin)
{
    ImGui::TextUnformatted(pin->get_object()->name.c_str());
    ImGui::SameLine();
    draw_pin_icon(pin->get_state()->pin_color, !m_ro_mode && pin->get_state()->links > 0);
    ImGui::NewLine();
}

void WorkflowDrawer::draw_link(const data::Link* link)
{
    ed::Link(link->get_graph_id(), link->get_input()->get_graph_id(), link->get_output()->get_graph_id(), link->get_input()->get_state()->pin_color);
}

void WorkflowDrawer::draw_pin_icon(ImU32 color, bool connected)
{
    const auto draw_list = ImGui::GetWindowDrawList();
    const auto cursorPos = ImGui::GetCursorScreenPos();
    const ImVec2 center = ImVec2(cursorPos.x + g_pin_radius, cursorPos.y + ImGui::GetTextLineHeight() / 2);

    if (connected)
        draw_list->AddCircleFilled(center, g_pin_radius, color);
    else
        draw_list->AddCircle(center, g_pin_radius, color, 0, 1.0f);
}

void WorkflowDrawer::draw_input_widget(data::PinInstance* pin, float max_w)
{
    if (m_ro_mode)
    {
        ImGui::Indent(g_pin_indent);
        ImGui::Dummy(ImVec2(g_pin_width, 0));
    }
    else
    {
        draw_pin_icon(pin->get_state()->converted ? pin->get_state()->pin_color : m_clr_node_widget_pin, false);
        ImGui::Dummy(ImVec2(g_pin_width, 0));
    }

    ImGui::SameLine();
    draw_widget(pin, max_w - g_pin_indent * 2 - g_pin_width * 2 - ImGui::GetStyle().ItemSpacing.x * 2);

    if (m_ro_mode)
        ImGui::Unindent(g_pin_indent);
}

void WorkflowDrawer::draw_widget(data::PinInstance* pin, float max_w)
{
    auto obj = pin->get_object();
    auto state = pin->get_state();
    bool control_after_generate = false;

    switch (state->type)
    {
    case data::PinType::INT:
        if (obj->is_unsigned)
            draw_widget_uint(pin, max_w);
        else
            draw_widget_int(pin, max_w);
        control_after_generate = obj->control_after_generate;
        break;
    case data::PinType::FLOAT:
        draw_widget_float(pin, max_w);
        control_after_generate = obj->control_after_generate;
        break;
    case data::PinType::BOOLEAN:
        draw_widget_bool(pin, max_w);
        break;
    case data::PinType::STRING:
        if (obj->multiline)
            draw_widget_str_ml(pin, max_w);
        else
            draw_widget_str(pin, max_w);
        break;
    case data::PinType::NUMBER:
        draw_widget_num(pin, max_w);
        break;
    case data::PinType::COMBO:
        draw_widget_combo(pin, max_w);
        break;
    case data::PinType::COLOR:
        draw_widget_color(pin, max_w);
        break;
    }

    if (control_after_generate)
    { }
}

void WorkflowDrawer::draw_widget_int(data::PinInstance* pin, float max_w)
{
    auto obj = pin->get_object();
    data::PinState* state = pin->get_state();

    int64_t& val = data::Pin::get_int(state->value);
    int64_t step = data::Pin::get_int(obj->step);
    int64_t min = data::Pin::get_int(obj->min);
    int64_t max = data::Pin::get_int(obj->max);

    ImGui::SetNextItemWidth(max_w);
    bool changed = false;

    switch (g::pOptsManager->wf_get_widget_input_numbers())
    {
    case WidgetInput::direct:
        changed = ImGuiEx::InputIntSmart("", &val, step, min, max, obj->precision_named, obj->precision);
        break;
    case WidgetInput::drag:
        changed = ImGui::DragScalar("", ImGuiDataType_S64, &val, 1.0f, &min, &max, obj->precision_named.c_str());
        break;
    case WidgetInput::slider:
    default:
        changed = ImGui::SliderScalar("", ImGuiDataType_S64, &val, &min, &max, obj->precision_named.c_str());
        break;
    }

    if (changed)
    {
        if (val < min) val = min;
        if (val > max) val = max;
    }
}

void WorkflowDrawer::draw_widget_uint(data::PinInstance* pin, float max_w)
{
    auto obj = pin->get_object();
    data::PinState* state = pin->get_state();

    uint64_t& val = data::Pin::get_uint(state->value);
    uint64_t step = data::Pin::get_uint(obj->step);
    uint64_t min = data::Pin::get_uint(obj->min);
    uint64_t max = data::Pin::get_uint(obj->max);

    ImGui::SetNextItemWidth(max_w);
    bool changed = false;

    switch (g::pOptsManager->wf_get_widget_input_numbers())
    {
    case WidgetInput::direct:
        changed = ImGuiEx::InputUIntSmart("", &val, step, min, max, obj->precision_named, obj->precision);
        break;
    case WidgetInput::slider:
    case WidgetInput::drag:
    default:
        changed = ImGui::DragScalar("", ImGuiDataType_U64, &val, float(INT_MAX), &min, &max, obj->precision_named.c_str());
        break;
    }

    if (changed)
    {
        if (val < min) val = min;
        if (val > max) val = max;
    }
}

void WorkflowDrawer::draw_widget_float(data::PinInstance* pin, float max_w)
{
    auto obj = pin->get_object();
    data::PinState* state = pin->get_state();

    double& val = data::Pin::get_float(state->value);
    double min = data::Pin::get_float(obj->min);
    double max = data::Pin::get_float(obj->max);
    double step = data::Pin::get_float(obj->step);

    ImGui::SetNextItemWidth(max_w);
    bool changed = false;

    switch (g::pOptsManager->wf_get_widget_input_numbers())
    {
    case WidgetInput::direct:
        changed = ImGuiEx::InputDoubleSmart("", &val, step, min, max, obj->precision_named, obj->precision);
        break;
    case WidgetInput::drag:
        changed = ImGui::DragScalar("", ImGuiDataType_Double, &val, 1.0f, &min, &max, obj->precision_named.c_str());
        break;
    case WidgetInput::slider:
    default:
        changed = ImGui::SliderScalar("", ImGuiDataType_Double, &val, &min, &max, obj->precision_named.c_str());
        break;
    }

    if (changed)
    {
        if (val < min) val = min;
        if (val > max) val = max;
    }
}

void WorkflowDrawer::draw_widget_bool(data::PinInstance* pin, float max_w)
{
    //auto obj = pin->get_object();
    data::PinState* state = pin->get_state();

    bool& val = data::Pin::get_bool(state->value);

    ImGui::SetNextItemWidth(max_w);
    bool changed = false;
/*
    switch (g::pOptsManager->wf_get_widget_input_bools())
    {
    case WidgetInput::radio:
        {
            const auto pos = ImGui::GetCursorPosX();
            const bool changed1 = ImGui::RadioButton(obj->label_off_named.c_str(), !val);
            ImGui::SetCursorPosX(pos);
            const bool changed2 = ImGui::RadioButton(obj->label_on_named.c_str(), val);
            changed = changed1 || changed2;
        }
        break;
    case WidgetInput::combo:
        {
            const char* items[] = { obj->label_off_named.c_str(), obj->label_on_named.c_str() };
            int current = val ? 1 : 0;
            changed = ImGui::Combo("", &current, items, 2);
        }
        break;
    case WidgetInput::checkbox:
        {
            bool temp = val;
            changed = ImGui::Checkbox(val ? obj->label_on_named.c_str() : obj->label_off_named.c_str(), &temp);
        }
        break;
    case WidgetInput::slider:
    default:
        {
            uint8_t uval = val ? 1 : 0;
            uint8_t min = 0, max = 1;
            changed = ImGui::SliderScalar("", ImGuiDataType_U8, &uval, &min, &max, val ? obj->label_on_named.c_str() : obj->label_off_named.c_str(), ImGuiSliderFlags_NoInput);
        }
        break;
    }
*/
    if (changed)
    {
        val = !val;
    }
}

void WorkflowDrawer::draw_widget_str(data::PinInstance* /*pin*/, float /*max_w*/)
{}
void WorkflowDrawer::draw_widget_str_ml(data::PinInstance* /*pin*/, float /*max_w*/)
{}
void WorkflowDrawer::draw_widget_num(data::PinInstance* /*pin*/, float /*max_w*/)
{}
void WorkflowDrawer::draw_widget_combo(data::PinInstance* /*pin*/, float /*max_w*/)
{}
void WorkflowDrawer::draw_widget_color(data::PinInstance* /*pin*/, float /*max_w*/)
{}

} // namespace ui
