
#include "ui/panels/panel_node_inspector.h"
#include "core/managers/settings_manager.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "ui/graph-n_flow.h"
#include "platform/main.h"

namespace ui
{

PanelNodeInspector::PanelNodeInspector()
    : PanelBase(VisibleObject::PanelNodeInspector, "Node Inspector", "95E5A1E7883E495E8712FA33A123F3BB")
    , m_clr_node_header_bg(g::pOptsManager->flow_get_color(VisibleObject::WfNodeHeaderBg))
    , m_clr_node_header_text(g::pOptsManager->flow_get_color(VisibleObject::WfNodeHeaderText))
    , m_clr_node_body_bg(g::pOptsManager->flow_get_color(VisibleObject::WfNodeBodyBg))
    , m_clr_node_widget_pin(g::pOptsManager->flow_get_color(VisibleObject::WfNodeWidgetPin))
    , m_clr_node_pin_text(g::pOptsManager->flow_get_color(VisibleObject::WfNodePinText))
    , m_clr_node_border(g::pOptsManager->flow_get_color(VisibleObject::WfNodeBorder))
{}

PanelNodeInspector::~PanelNodeInspector() = default;

void PanelNodeInspector::init()
{
    LogF;
    PanelBase::init();

    g::pFlow->subscribe_comfyui_validity_state(this);

    data()->clr_bg = g::pOptsManager->flow_get_color(VisibleObject::PanelNodeInspector);
    data()->clr_descr_frame = g::pOptsManager->flow_get_color(VisibleObject::PanelNodeInspectorDescrFrame);
    data()->clr_selected_node = g::pOptsManager->flow_get_color(VisibleObject::PanelNodesSelectedNode);

    m_wf = g::pFlow->create_workflow();
}

void PanelNodeInspector::add_inspecting_node(data::Node* node)
{    
    if (nullptr == node)
        return;
    if (std::find_if(m_inspecting.begin(), m_inspecting.end(), [node](const Data& d) { return d.node->get_object()->get_id() == node->get_id(); }) != m_inspecting.end())
        return;
    Log("Add node '" << node->name << "' id=" << node->get_id());

    Data data;
    data.node = m_wf->add_node(node);
    data.ideal_size = data.node->calc_node_width();
    data.descr_wrapper.set_font(g::pMain->get_regular_font());
    if (!node->description.empty())
        data.descr_wrapper.set_raw_text(&node->description);

    m_inspecting.emplace_back(std::move(data));
}

void PanelNodeInspector::remove_inspecting_node(data::Node* node)
{
    Log("Remove node '" << node->name << "' id=" << node->get_id());

    const auto it = std::find_if(m_inspecting.begin(), m_inspecting.end(), [node](const Data& d) { return d.node->get_object()->get_id() == node->get_id(); });
    if (it == m_inspecting.end())
    {
        LogInternalError(ExcepCategory::inconsistent_map, "Can't remove node");
    }
    else
    {
        m_inspecting.erase(it);
    }
}

void PanelNodeInspector::reset_inspecting_nodes()
{
    Log("Remove all nodes");
    m_inspecting.clear();
}

void PanelNodeInspector::invalidate_comfyui_objects()
{
    reset_inspecting_nodes();
}

DlgCode PanelNodeInspector::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
    {
        if (m_inspecting.empty())
        {
            ImGuiEx::Spacing(3);
            ImGui::Text(ICON_LC_TANGENT " Select node(s) to inspect");
            return end_panel(true);
        }
        else
        {
            show_nodes();
        }
    }

    return end_panel(opened);
}

void PanelNodeInspector::show_nodes()
{
    ImGuiStyle& style = ImGui::GetStyle();
    const auto avail_w = ImGui::GetContentRegionAvail().x;
    constexpr float min_descr_w = 200.0f;
    float curr_y = 0.0f;

    for (size_t i = 0; i < m_inspecting.size(); i++)
    {
        auto& node = m_inspecting[i];
        auto node_obj = node.node->get_object();

        const float col_w = std::max(node.ideal_size, min_descr_w);

        ImGui::PushID(node_obj->get_id_str().c_str());
        ImGui::BeginGroup();

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, data()->clr_selected_node);
        ImGui::TextUnformatted(node.node->get_name().c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();
        show_preview(node.node, col_w);
        show_description(node.descr_wrapper, col_w);

        ImGui::EndGroup();
        ImGui::PopID();

        if (i + 1 < m_inspecting.size())
        {
            const float last_group_w = ImGui::GetItemRectSize().x;
            curr_y += last_group_w;
            const float next_col_w = std::max(m_inspecting[i + 1].ideal_size, min_descr_w);
            if (curr_y + style.ItemSpacing.x * 2 + next_col_w < avail_w)
                ImGui::SameLine();
            else
                curr_y = 0.0f;
        }
    }
}

void PanelNodeInspector::show_description(TextWrapperRO& descr_wrapper, float max_w)
{
    auto raw = descr_wrapper.get_raw_text();
    if (raw != nullptr && *raw != 0)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, data()->clr_bg);
       // ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, data()->clr_descr_frame);

        ImGuiEx::InputTextMultiline("##NodeDescription", descr_wrapper.get_wrapped_text(),
            ImVec2(max_w, descr_wrapper.get_wrapped_height()),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
        descr_wrapper.re_calc(ImGui::GetItemRectSize());

        ImGui::PopStyleColor(2);
       // ImGui::PopStyleVar();
        ImGuiEx::Spacing(3);
    }
}

//float temp_pos = 0.0f;
//#define TEMP_POS        temp_pos = ImGui::GetCursorPosX();

void PanelNodeInspector::show_preview(data::NodeInstance* node_inst, float max_w)
{
    max_w = node_inst->recalc_node_widths(max_w, false /*allow_extend*/, false /*allow_compress*/, true /*use_big_pin_icon*/);
    m_curr_node_left_offset = ImGui::GetCursorPosX();

    ImGuiEx::PushID(node_inst->get_graph_id());

    // --- NODE FRAME ---

    const auto draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);
    draw_list->ChannelsSetCurrent(1);

    ImGui::BeginGroup();            // 1 -> node
    ImGui::Dummy(ImVec2(node_inst->w_node, w::v_spacing));

    // --- HEADER ---

    ImGui::BeginGroup();            // 2 -> header
    draw_node_header(node_inst);
    ImGui::EndGroup();              // 2 -> header

    ImGui::Dummy(ImVec2(node_inst->w_node, w::v_spacing));

    // --- PINS - INPUT ---

    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_pin_text);
    ImGui::BeginGroup();            // 3 -> in pins

    const auto& input = node_inst->m_input;
    for (auto pin : input)
    {
        if (nullptr == pin || !pin->is_valid())
        {
            LogError(ExcepCategory::logic, "Null pin");
        }
        else if (!pin->get_object()->is_widget())
        {
            ImGuiEx::PushID(pin->get_graph_id());
            draw_input_pin(pin);
            ImGui::PopID();
        }
    }
    ImGui::EndGroup();              // 3 -> in pins

    // --- SPACER ---

    ImGui::SameLine(0.0f, 0.0f);
    if (node_inst->w_center > 0.0f)
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset + w::ico_max + w::pin_r_offset + node_inst->w_pin_in + node_inst->w_center);

    // --- PINS - OUTPUT ---

    ImGui::BeginGroup();            // 4 -> out pins
    const auto out_pins_pos = ImGui::GetCursorPosX();

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
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset + w::ico_max + w::pin_r_offset + node_inst->w_pin_in + node_inst->w_center +
                node_inst->w_pin_out - pin->get_object()->w_name);
            draw_output_pin(pin);
            ImGui::PopID(); // pin id
        }
    }
    ImGui::EndGroup();              // 4 -> out pins

    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(node_inst->w_node, w::v_spacing * 2));

    if (node_inst->m_has_widgets)
    {
        // --- WIDGETS ---

        ImGui::BeginGroup();            // 5 -> widgets

        size_t advanced = 0;
        for (auto pin : input)
        {
            if (pin->m_state->advanced)
                advanced++;
            else if (pin->get_object()->is_widget())
            {
                ImGuiEx::PushID(pin->get_graph_id());
                draw_input_widget(pin, node_inst->w_widget_name, node_inst->w_widget_value, false);

                if (pin->get_object()->control_after_generate)
                {
                    draw_input_widget(pin, node_inst->w_widget_name, node_inst->w_widget_value, true);
                }
                ImGui::PopID(); // widget id
            }
        }
        if (advanced > 0)
        {
            draw_widget_separator(w::pin_l_offset + w::ico_max + w::pin_r_offset, node_inst->w_separator);
            for (auto pin : input)
            {
                if (pin->m_state->advanced)
                {
                    ImGuiEx::PushID(pin->get_graph_id());
                    draw_input_widget(pin, node_inst->w_widget_name, node_inst->w_widget_value, false);

                    if (pin->get_object()->control_after_generate)
                    {
                        draw_input_widget(pin, node_inst->w_widget_name, node_inst->w_widget_value, true);
                    }
                    ImGui::PopID(); // widget id
                }
            }
        }
        ImGui::EndGroup();              // 5 -> widgets
        ImGui::Dummy(ImVec2(node_inst->w_node, 10.0f));
    }

    // --- FINALIZE ---

    ImGui::EndGroup();              // -> node

    // Channel 0
    const ImVec2 node_min = ImGui::GetItemRectMin();
    const ImVec2 node_max = ImGui::GetItemRectMax();

    draw_list->ChannelsSetCurrent(0);

    // Body
    draw_list->AddRectFilled(node_min, node_max, m_clr_node_body_bg, 12.0f);

    // Header
    const float header_h = ImGui::GetTextLineHeight() + 8.0f + (w::v_spacing / 2.0f);
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

    ImGui::PopID(); // node_inst id
}

void PanelNodeInspector::draw_node_header(data::NodeInstance* node)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + node->w_node_name_off);
    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_header_text);
    ImGui::TextUnformatted(node->get_name().c_str());
    ImGui::PopStyleColor();
}

void PanelNodeInspector::draw_input_pin(data::PinInstance* pin)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset);
    draw_pin_icon(pin->m_state->pin_color);
    ImGui::SameLine(0.0f, w::pin_r_offset);
    ImGui::TextUnformatted(pin->get_object()->name.c_str());
}

void PanelNodeInspector::draw_output_pin(data::PinInstance* pin)
{
    ImGui::TextUnformatted(pin->get_object()->name.c_str());
    ImGui::SameLine(0, w::pin_r_offset);
    draw_pin_icon(pin->m_state->pin_color);
}

void PanelNodeInspector::draw_pin_icon(ImU32 color)
{
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(ICON_LC_CIRCLE_SMALL);
    ImGui::PopStyleColor();

    //const auto cursorPos = ImGui::GetCursorScreenPos();
    //const ImVec2 center = ImVec2(cursorPos.x + mngr::SettingsManager::wf_get_pin_radius(), cursorPos.y + ImGui::GetTextLineHeight() / 2);

    //const auto draw_list = ImGui::GetWindowDrawList();
    //draw_list->AddCircle(center, mngr::SettingsManager::wf_get_pin_radius(), color, 0, 1.0f);
}

void PanelNodeInspector::draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width, bool draw_control_after_generate /*= false*/)
{
    if (draw_control_after_generate)
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset + w::ico_max + w::pin_r_offset);
        draw_widget_control_after_generate(pin, widget_name_width, widget_value_width);
    }
    else
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset);
        ImGui::TextUnformatted(pin->m_state->advanced ? ICON_LC_EYE_CLOSED : ICON_LC_EYE);

        const ImVec2 size = ImGui::GetItemRectSize();
        ImGui::SameLine();
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset);
        if (ImGui::InvisibleButton("##pin", size))
            pin->m_state->advanced = !pin->m_state->advanced;

        ImGui::SameLine(0, w::pin_r_offset);
        draw_widget(pin, widget_name_width, widget_value_width);
    }
}

void PanelNodeInspector::draw_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    switch (pin->m_state->type)
    {
    case data::PinType::INT:
        if (pin->get_object()->is_unsigned)
            draw_widget_uint(pin, widget_name_width, widget_value_width);
        else
            draw_widget_int(pin, widget_name_width, widget_value_width);
        break;
    case data::PinType::FLOAT:
        draw_widget_float(pin, widget_name_width, widget_value_width);
        break;
    case data::PinType::BOOLEAN:
        draw_widget_bool(pin, widget_name_width, widget_value_width);
        break;
    case data::PinType::STRING:
        if (pin->get_object()->multiline)
            draw_widget_str_ml(pin, widget_name_width, widget_value_width);
        else
            draw_widget_str(pin, widget_name_width, widget_value_width);
        break;
    case data::PinType::NUMBER:
        draw_widget_num(pin, widget_name_width, widget_value_width);
        break;
    case data::PinType::COMBO:
        draw_widget_combo(pin, widget_name_width, widget_value_width);
        break;
    case data::PinType::COLOR:
        draw_widget_color(pin, widget_name_width, widget_value_width);
        break;
    }
}

void PanelNodeInspector::draw_widget_int(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    int64_t val = data::Pin::get_int(state->value);
    int64_t step = data::Pin::get_int(obj->step);
    int64_t min = data::Pin::get_int(obj->min);
    int64_t max = data::Pin::get_int(obj->max);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

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

void PanelNodeInspector::draw_widget_uint(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    uint64_t val = data::Pin::get_uint(state->value);
    uint64_t step = data::Pin::get_uint(obj->step);
    uint64_t min = data::Pin::get_uint(obj->min);
    uint64_t max = data::Pin::get_uint(obj->max);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

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

void PanelNodeInspector::draw_widget_float(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    double val = data::Pin::get_float(state->value);
    double min = data::Pin::get_float(obj->min);
    double max = data::Pin::get_float(obj->max);
    double step = data::Pin::get_float(obj->step);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

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

void PanelNodeInspector::draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;
    bool val = data::Pin::get_bool(state->value);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 0.0f);

    switch (g::pOptsManager->wf_get_widget_input_bools())
    {
        //case WidgetInput::switcher:
        //    ImGuiEx::BooleanSwitch("", val ? obj->label_on.c_str() : obj->label_off.c_str(), &val, max_w);
        //    break;
    case WidgetInput::toggle:
        ImGui::SetNextItemWidth(widget_value_width);
        ImGuiEx::BooleanButtons2("", val ? obj->label_on.c_str() : obj->label_off.c_str(), &val);
        break;
    case WidgetInput::select:
    default:
        ImGuiEx::BooleanToggleLED(val ? obj->label_on.c_str() : obj->label_off.c_str(), &val, false, false, widget_value_width);
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void PanelNodeInspector::draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    auto& placeholder = (obj->placeholder.empty() ? obj->name : obj->placeholder);
    std::string val = data::Pin::get_string(pin->m_state->value);

    ImGui::SetNextItemWidth(widget_name_width + widget_value_width);
    ImGuiEx::InputTextWithHint("", placeholder.c_str(), &val);

    bool tt_mod = false;
    if (!val.empty())
        tt_mod = ImGuiEx::TooltipOnLastControl(placeholder.c_str(), ImGuiHoveredFlags_DelayNone, true);

    if (!tt_mod)
        ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void PanelNodeInspector::draw_widget_str_ml(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    auto& placeholder = (obj->placeholder.empty() ? obj->name : obj->placeholder);
    std::string val = data::Pin::get_string(pin->m_state->value);

    ImGui::SetNextItemWidth(widget_name_width + widget_value_width);
    ImGuiEx::InputTextMultilineWithHint("", placeholder.c_str(), &val);

    bool tt_mod = false;
    if (!val.empty())
        tt_mod = ImGuiEx::TooltipOnLastControl(placeholder.c_str(), ImGuiHoveredFlags_DelayNone, true);

    if (!tt_mod)
        ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void PanelNodeInspector::draw_widget_num(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    draw_widget_int(pin, widget_name_width, widget_value_width);
}

void PanelNodeInspector::draw_widget_combo(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);
    ImGui::Combo("", &pin->m_state->combo_index, obj->combo_items_ptr.data(), cast::to_i32(obj->combo_items_ptr.size()));

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void PanelNodeInspector::draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    auto colors = state->colors;
    if (ImGui::ColorEdit4("", colors,
        ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop |
        ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoTooltip | (obj->alpha_enabled ? 0 : ImGuiColorEditFlags_NoAlpha)))
    {
        state->value = data::Pin::colors_to_int(state->colors);
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str());
}

void PanelNodeInspector::draw_widget_control_after_generate(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    if (obj->control_after_generate)
    {
        auto& state = pin->m_state;
        if (data::PinType::INT == state->type || data::PinType::FLOAT == state->type || data::PinType::NUMBER == state->type)
        {
            ImGui::SetNextItemWidth(widget_name_width);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::InputText("##CAGControl", "control", ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();

            ImGui::SameLine(0.0f, 0.0f);
            ImGui::SetNextItemWidth(widget_value_width);

            int curr = cast::to_i32(state->control_after_generate_value);
            ImGui::Combo("##CAGCombo", &curr, data::ControlAfterGenerateStr, sizeof(data::ControlAfterGenerateStr) / sizeof(data::ControlAfterGenerateStr[0]));
        }
    }
}

void PanelNodeInspector::draw_widget_separator(float offset, float width)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + offset);
    ImGuiEx::Separator(IM_COL32(0, 180, 180, 200), width, w::hidden_widgets.c_str(), w::hidden_widgets.size(), w::hidden_widgets_len);
}

} // namespace ui
