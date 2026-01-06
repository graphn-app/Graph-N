
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
    : m_clr_node_header_bg(g::pOptsManager->flow_get_color(VisibleObject::WfNodeHeaderBg))
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
            LogError(ExcepCategory::logic, "Null node");
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
                LogError(ExcepCategory::logic, "Null pin");
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
        ImGui::Dummy(ImVec2(mngr::SettingsManager::wf_get_pin_center_x_offset() * 2, 0.0f));
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
                LogError(ExcepCategory::logic, "Null pin");
            }
            else
            {
                ImGuiEx::PushID(pin->get_graph_id());

                const float start_pos = node_inst->get_node_width() - mngr::SettingsManager::wf_get_pin_indent() - mngr::SettingsManager::wf_get_pin_width() - pin->get_object()->name_width - ImGui::GetStyle().ItemSpacing.x;
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

void WorkflowDrawer::draw_node(data::NodeInstance* /*node*/)
{
    /*auto obj = node->get_object();

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


    float spacer = (node->get_node_width() - obj->node_name_width) * 0.5f;
    spacer -= ed::GetStyle().NodePadding.x;
    if (spacer > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spacer);

    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_header_text);
    ImGui::TextUnformatted(obj->display_name.empty() ? obj->name.c_str() : obj->display_name.c_str());
    ImGui::PopStyleColor();*/
}

void WorkflowDrawer::draw_input_pin(data::PinInstance* pin)
{
    auto obj = pin->get_object();
    draw_pin_icon(pin->m_state->pin_color, pin->m_state->links > 0, false);
    ImGui::Dummy(ImVec2(mngr::SettingsManager::wf_get_pin_width(), 0));
    ImGui::SameLine();
    ImGui::TextUnformatted(obj->name.c_str());
}

void WorkflowDrawer::draw_output_pin(data::PinInstance* pin)
{
    ImGui::TextUnformatted(pin->get_object()->name.c_str());
    ImGui::SameLine();
    draw_pin_icon(pin->m_state->pin_color, pin->m_state->links > 0, false);
    ImGui::NewLine();
}

void WorkflowDrawer::draw_link(const data::Link* link)
{
    ed::Link(link->get_graph_id(), link->get_input()->get_graph_id(), link->get_output()->get_graph_id(), link->get_input()->m_state->pin_color);
}

void WorkflowDrawer::draw_pin_icon(ImU32 color, bool connected, bool for_widget)
{
    const auto draw_list = ImGui::GetWindowDrawList();
    const auto cursorPos = ImGui::GetCursorScreenPos();

    ImVec2 center;
    if (for_widget)
        center = ImVec2(cursorPos.x + mngr::SettingsManager::wf_get_pin_radius(), cursorPos.y + ImGui::GetTextLineHeight() / 3 * 2);
    else
        center = ImVec2(cursorPos.x + mngr::SettingsManager::wf_get_pin_radius(), cursorPos.y + ImGui::GetTextLineHeight() / 2);

    if (connected)
        draw_list->AddCircleFilled(center, mngr::SettingsManager::wf_get_pin_radius(), color);
    else
        draw_list->AddCircle(center, mngr::SettingsManager::wf_get_pin_radius(), color, 0, 1.0f);
}

void WorkflowDrawer::draw_input_widget(data::PinInstance* pin, float r_group_width, float max_widget_width, bool draw_control_after_generate /*= false*/)
{
    const auto pos = ImGui::GetCursorPosX();
    const auto spacing = ImGui::GetStyle().ItemSpacing.x;
    const auto l_offset = pos + mngr::SettingsManager::wf_get_pin_indent() + mngr::SettingsManager::wf_get_pin_width() + spacing;
    const auto max_width = max_widget_width - l_offset - mngr::SettingsManager::wf_get_pin_width() - mngr::SettingsManager::wf_get_pin_indent() - spacing;
    const auto l_width = max_width - r_group_width;
    const auto r_offset = pos + l_offset + l_width - spacing;

    if (draw_control_after_generate)
        ImGui::Indent(mngr::SettingsManager::wf_get_pin_indent());
    else
        draw_pin_icon(pin->m_state->converted ? pin->m_state->pin_color : m_clr_node_widget_pin, pin->m_state->links > 0, true);
    ImGui::Dummy(ImVec2(mngr::SettingsManager::wf_get_pin_width(), 0));

    ImGui::SameLine();
    if (!draw_control_after_generate)
        draw_widget(pin, l_width, r_offset, r_group_width, max_width);
    else
        draw_widget_control_after_generate(pin, l_width, r_offset, r_group_width, max_width);

    if (draw_control_after_generate)
        ImGui::Unindent(mngr::SettingsManager::wf_get_pin_indent());
}

void WorkflowDrawer::draw_widget(data::PinInstance* pin, float l_group_width, float r_offset, float r_group_width, float max_widget_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    switch (state->type)
    {
    case data::PinType::INT:
        if (obj->is_unsigned)
            draw_widget_uint(pin, l_group_width, r_offset, r_group_width, max_widget_width);
        else
            draw_widget_int(pin, l_group_width, r_offset, r_group_width, max_widget_width);
        break;
    case data::PinType::FLOAT:
        draw_widget_float(pin, l_group_width, r_offset, r_group_width, max_widget_width);
        break;
    case data::PinType::BOOLEAN:
        draw_widget_bool(pin, l_group_width, r_offset, r_group_width, max_widget_width);
        break;
    case data::PinType::STRING:
        if (obj->multiline)
            draw_widget_str_ml(pin, l_group_width, r_offset, r_group_width, max_widget_width);
        else
            draw_widget_str(pin, l_group_width, r_offset, r_group_width, max_widget_width);
        break;
    case data::PinType::NUMBER:
        draw_widget_num(pin, l_group_width, r_offset, r_group_width, max_widget_width);
        break;
    case data::PinType::COMBO:
        draw_widget_combo(pin, l_group_width, r_offset, r_group_width, max_widget_width);
        break;
    case data::PinType::COLOR:
        draw_widget_color(pin, l_group_width, r_offset, r_group_width, max_widget_width);
        break;
    }
}

void WorkflowDrawer::draw_widget_int(data::PinInstance* pin, float l_group_width, float r_offset, float r_group_width, float /*max_widget_width*/)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    int64_t& val = data::Pin::get_int(state->value);
    int64_t step = data::Pin::get_int(obj->step);
    int64_t min = data::Pin::get_int(obj->min);
    int64_t max = data::Pin::get_int(obj->max);

    ImGui::SetNextItemWidth(l_group_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetCursorPosX(r_offset);
    ImGui::SetNextItemWidth(r_group_width);

    switch (g::pOptsManager->wf_get_widget_input_numbers())
    {
    case WidgetInput::direct:
        ImGuiEx::InputIntSmart("", &val, step, min, max, obj->precision, obj->precision);
        break;
    case WidgetInput::drag:
        ImGuiEx::DragIntSmart("", &val, min, max, obj->precision.c_str());
        break;
    case WidgetInput::slider:
    default:
        ImGuiEx::SliderIntSmart("", &val, min, max, obj->precision.c_str());
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void WorkflowDrawer::draw_widget_uint(data::PinInstance* pin, float l_group_width, float r_offset, float r_group_width, float /*max_widget_width*/)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    uint64_t& val = data::Pin::get_uint(state->value);
    uint64_t step = data::Pin::get_uint(obj->step);
    uint64_t min = data::Pin::get_uint(obj->min);
    uint64_t max = data::Pin::get_uint(obj->max);

    ImGui::SetNextItemWidth(l_group_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetCursorPosX(r_offset);
    ImGui::SetNextItemWidth(r_group_width);

    switch (g::pOptsManager->wf_get_widget_input_numbers())
    {
    case WidgetInput::direct:
        ImGuiEx::InputUIntSmart("", &val, step, min, max, obj->precision, obj->precision);
        break;
    case WidgetInput::slider:
    case WidgetInput::drag:
    default:
        ImGuiEx::DragUIntSmart("", &val, min, max, obj->precision.c_str());
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void WorkflowDrawer::draw_widget_float(data::PinInstance* pin, float l_group_width, float r_offset, float r_group_width, float /*max_widget_width*/)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    double& val = data::Pin::get_float(state->value);
    double min = data::Pin::get_float(obj->min);
    double max = data::Pin::get_float(obj->max);
    double step = data::Pin::get_float(obj->step);

    ImGui::SetNextItemWidth(l_group_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetCursorPosX(r_offset);
    ImGui::SetNextItemWidth(r_group_width);

    switch (g::pOptsManager->wf_get_widget_input_numbers())
    {
    case WidgetInput::direct:
        ImGuiEx::InputDoubleSmart("", &val, step, min, max, obj->precision, obj->precision);
        break;
    case WidgetInput::drag:
        ImGuiEx::DragDoubleSmart("", &val, min, max, obj->precision.c_str());
        break;
    case WidgetInput::slider:
    default:
        ImGuiEx::SliderDoubleSmart("", &val, min, max, obj->precision.c_str());
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void WorkflowDrawer::draw_widget_bool(data::PinInstance* pin, float l_group_width, float r_offset, float r_group_width, float /*max_widget_width*/)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;
    bool& val = data::Pin::get_bool(state->value);

    ImGui::SetNextItemWidth(l_group_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetCursorPosX(r_offset);

    switch (g::pOptsManager->wf_get_widget_input_bools())
    {
    //case WidgetInput::switcher:
    //    ImGuiEx::BooleanSwitch("", val ? obj->label_on.c_str() : obj->label_off.c_str(), &val, max_w);
    //    break;
    case WidgetInput::toggle:
        ImGui::SetNextItemWidth(r_group_width);
        ImGuiEx::BooleanButtons2("", val ? obj->label_on.c_str() : obj->label_off.c_str(), &val);
        break;
    case WidgetInput::select:
    default:
        ImGuiEx::BooleanToggleLED(val ? obj->label_on.c_str() : obj->label_off.c_str(), &val, false, false, r_group_width);
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void WorkflowDrawer::draw_widget_str(data::PinInstance* pin, float /*l_group_width*/, float /*r_offset*/, float /*r_group_width*/, float max_widget_width)
{
    auto obj = pin->get_object();

    auto& placeholder = (obj->placeholder.empty() ? obj->name : obj->placeholder);
    std::string& val = data::Pin::get_string(pin->m_state->value);

    ImGui::SetNextItemWidth(max_widget_width);
    ImGuiEx::InputTextWithHint("", placeholder.c_str(), &val);

    bool tt_mod = false;
    if (!val.empty())
        tt_mod = ImGuiEx::TooltipOnLastControl(placeholder.c_str(), ImGuiHoveredFlags_DelayNone, true);

    if (!tt_mod)
        ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void WorkflowDrawer::draw_widget_str_ml(data::PinInstance* pin, float /*l_group_width*/, float /*r_offset*/, float /*r_group_width*/, float max_widget_width)
{
    auto obj = pin->get_object();

    auto& placeholder = (obj->placeholder.empty() ? obj->name : obj->placeholder);
    std::string& val = data::Pin::get_string(pin->m_state->value);

    ImGui::SetNextItemWidth(max_widget_width);
    ImGuiEx::InputTextMultilineWithHint("", placeholder.c_str(), &val);

    bool tt_mod = false;
    if (!val.empty())
        tt_mod = ImGuiEx::TooltipOnLastControl(placeholder.c_str(), ImGuiHoveredFlags_DelayNone, true);

    if (!tt_mod)
        ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void WorkflowDrawer::draw_widget_num(data::PinInstance* pin, float l_group_width, float r_offset, float r_group_width, float max_widget_width)
{
    draw_widget_int(pin, l_group_width, r_offset, r_group_width, max_widget_width);
}

void WorkflowDrawer::draw_widget_combo(data::PinInstance* pin, float l_group_width, float r_offset, float r_group_width, float /*max_widget_width*/)
{
    auto obj = pin->get_object();

    ImGui::SetNextItemWidth(l_group_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetCursorPosX(r_offset);
    ImGui::SetNextItemWidth(r_group_width);

    if (ImGui::Combo("", &pin->m_state->combo_index, obj->combo_items_ptr.data(), cast::to_i32(obj->combo_items_ptr.size())))
    {
        pin->m_state->value = obj->list[pin->m_state->combo_index];
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void WorkflowDrawer::draw_widget_color(data::PinInstance* pin, float l_group_width, float r_offset, float r_group_width, float /*max_widget_width*/)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    ImGui::SetNextItemWidth(l_group_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetCursorPosX(r_offset);
    ImGui::SetNextItemWidth(r_group_width);

    if (ImGui::ColorEdit4("", state->colors,
        ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop |
        ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoTooltip | (obj->alpha_enabled ? 0 : ImGuiColorEditFlags_NoAlpha)))
    {
        state->value = data::Pin::colors_to_int(state->colors);
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void WorkflowDrawer::draw_widget_control_after_generate(data::PinInstance* pin, float l_group_width, float r_offset, float r_group_width, float /*max_widget_width*/)
{
    auto obj = pin->get_object();
    if (obj->control_after_generate)
    {
        auto& state = pin->m_state;
        if (data::PinType::INT == state->type || data::PinType::FLOAT == state->type || data::PinType::NUMBER == state->type)
        {
            ImGui::SetNextItemWidth(l_group_width);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::InputText("##CAGControl", "control", ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGui::SetCursorPosX(r_offset);
            ImGui::SetNextItemWidth(r_group_width);

            int curr = cast::to_i32(state->control_after_generate_value);            
            if (ImGui::Combo("##CAGCombo", &curr, data::ControlAfterGenerateStr, sizeof(data::ControlAfterGenerateStr) / sizeof(data::ControlAfterGenerateStr[0])))
            {
                state->control_after_generate_value = cast::to_enum<data::ControlAfterGenerate>(curr);
            }
        }
    }
}

} // namespace ui
