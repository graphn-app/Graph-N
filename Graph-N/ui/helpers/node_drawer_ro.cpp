
#include "ui/helpers/node_drawer_ro.h"
#include "core/managers/settings_manager.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "core/workflow/node_instance_ro.h"
#include "core/workflow/pin_instance.h"

namespace ui
{

RoNodeDrawer::RoNodeDrawer()
    : m_clr_node_header_bg(g::pOptsManager->color_load(AppColor::NodeHeaderBg))
    , m_clr_node_header_text(g::pOptsManager->color_load(AppColor::NodeHeaderText))
    , m_clr_node_body_bg(g::pOptsManager->color_load(AppColor::NodeBodyBg))
    , m_clr_node_pin_text(g::pOptsManager->color_load(AppColor::NodePinText))
    , m_clr_node_border(g::pOptsManager->color_load(AppColor::NodeBorder))
{
    LogF;
}

RoNodeDrawer::~RoNodeDrawer()
{
    LogF;
}

RoNodeDrawer::RoNodeDrawer(RoNodeDrawer&& other)
    : RoNodeDrawer()
{
    *this = std::move(other);
}

RoNodeDrawer& RoNodeDrawer::operator=(RoNodeDrawer&& other)
{
    if (this == &other)
        return *this;

    m_node = other.m_node;
    m_tt_mode = other.m_tt_mode;
    m_curr_node_left_offset = other.m_curr_node_left_offset;
    return *this;
}

float RoNodeDrawer::draw(data::NodeInstanceRo* node, bool tooltip_mode, float max_w)
{
    m_node = node;
    m_tt_mode = tooltip_mode;

    max_w = m_node->recalc_node_width(max_w, true, !m_tt_mode /*use_big_pin_icon*/);
    m_curr_node_left_offset = ImGui::GetCursorPosX();

    ImGui::PushID(m_node);

    // --- NODE FRAME ---

    const auto draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);
    draw_list->ChannelsSetCurrent(1);

    ImGui::BeginGroup();            // 1 -> node
    ImGui::Dummy(ImVec2(m_node->w_node, w::v_spacing));

    // --- HEADER ---

    ImGui::BeginGroup();            // 2 -> header
    draw_node_header(m_node);
    ImGui::EndGroup();              // 2 -> header

    ImGui::Dummy(ImVec2(m_node->w_node, w::v_spacing));

    // --- PINS - INPUT ---

    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_pin_text);
    ImGui::BeginGroup();            // 3 -> in pins

    const auto& input = m_node->get_inputs();
    for (auto pin : input)
    {
        if (nullptr == pin || !pin->is_valid())
        {
            LogError(ExcepCategory::Logic, "Null pin");
        }
        else if (!pin->get_object()->is_widget())
        {
            ImGui::PushID(pin);
            draw_input_pin(pin);
            ImGui::PopID();
        }
    }
    ImGui::EndGroup();              // 3 -> in pins

    // --- SPACER ---

    ImGui::SameLine(0.0f, 0.0f);
    if (m_node->w_center > 0.0f)
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro + w::ico_max + w::pin_r_offset + m_node->w_pin_in + m_node->w_center);

    // --- PINS - OUTPUT ---

    ImGui::BeginGroup();            // 4 -> out pins
    const auto out_pins_pos = ImGui::GetCursorPosX();

    const auto& output = m_node->get_outputs();
    for (auto pin : output)
    {
        if (nullptr == pin || !pin->is_valid())
        {
            LogError(ExcepCategory::Logic, "Null pin");
        }
        else
        {
            ImGui::PushID(pin);
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro + w::ico_max + w::pin_r_offset + m_node->w_pin_in + m_node->w_center +
                m_node->w_pin_out - pin->get_object()->w_name);
            draw_output_pin(pin);
            ImGui::PopID(); // pin id
        }
    }
    ImGui::EndGroup();              // 4 -> out pins

    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(m_node->w_node, w::v_spacing * 2));

    if (m_node->m_has_widgets)
    {
        // --- WIDGETS ---

        ImGui::BeginGroup();            // 5 -> widgets

        size_t advanced = 0;
        for (auto pin : input)
        {
            auto obj = pin->get_object();
            if (obj->advanced)
                advanced++;
            else if (obj->is_widget())
            {
                ImGui::PushID(pin);
                draw_input_widget(pin, m_node->w_widget_name, m_node->w_widget_value, false);

                if (obj->control_after_generate)
                {
                    draw_input_widget(pin, m_node->w_widget_name, m_node->w_widget_value, true);
                }
                ImGui::PopID(); // widget id
            }
        }
        if (advanced > 0)
        {
            draw_widget_separator(w::pin_l_offset_ro + w::ico_max + w::pin_r_offset, m_node->w_separator);
            for (auto pin : input)
            {
                auto obj = pin->get_object();
                if (obj->advanced)
                {
                    ImGui::PushID(pin);
                    draw_input_widget(pin, m_node->w_widget_name, m_node->w_widget_value, false);

                    if (obj->control_after_generate)
                    {
                        draw_input_widget(pin, m_node->w_widget_name, m_node->w_widget_value, true);
                    }
                    ImGui::PopID(); // widget id
                }
            }
        }
        ImGui::EndGroup();              // 5 -> widgets
        ImGui::Dummy(ImVec2(m_node->w_node, 10.0f));
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

    ImGui::PopID(); // m_node id
    return max_w;
}

void RoNodeDrawer::draw_node_header(data::NodeInstanceRo* node)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + node->w_node_name_off);
    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_header_text);
    ImGui::TextUnformatted(node->get_name().c_str(), node->get_name().c_str() + node->get_name().size());
    ImGui::PopStyleColor();
}

void RoNodeDrawer::draw_input_pin(data::PinInstance* pin)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro);
    draw_pin_icon(pin->m_state->pin_color);
    ImGui::SameLine(0.0f, w::pin_r_offset);
    ImGui::TextUnformatted(pin->get_object()->name.c_str(), pin->get_object()->name.c_str() + pin->get_object()->name.size());
}

void RoNodeDrawer::draw_output_pin(data::PinInstance* pin)
{
    ImGui::TextUnformatted(pin->get_object()->name.c_str(), pin->get_object()->name.c_str() + pin->get_object()->name.size());
    ImGui::SameLine(0, w::pin_r_offset);
    draw_pin_icon(pin->m_state->pin_color);
}

void RoNodeDrawer::draw_pin_icon(ImU32 color)
{
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(ICON_LC_CIRCLE_SMALL);
    ImGui::PopStyleColor();

    //const auto cursorPos = ImGui::GetCursorScreenPos();
    //const ImVec2 center = ImVec2(cursorPos.x + mngr::SettingsManager::wf_get_pin_radius(), cursorPos.y + ImGui::GetTextLineHeight() / 2);

    //const auto draw_list = ImGui::GetWindowDrawList();
    //draw_list->AddCircle(center, mngr::SettingsManager::wf_get_pin_radius(), color, 0, 1.0f);
}

void RoNodeDrawer::draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width, bool draw_control_after_generate /*= false*/)
{
    if (draw_control_after_generate)
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro + w::ico_max + w::pin_r_offset);
        draw_widget_control_after_generate(pin, widget_name_width, widget_value_width);
    }
    else
    {
        if (m_tt_mode)
        {
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro + w::ico_max + w::pin_r_offset);
        }
        else
        {
            auto obj = pin->get_object();

            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro);
            ImGui::TextUnformatted(obj->advanced ? ICON_LC_EYE_CLOSED : ICON_LC_EYE);

            const ImVec2 size = ImGui::GetItemRectSize();
            ImGui::SameLine();
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro);
            if (ImGui::InvisibleButton("##pin", size))
            {
                obj->advanced = !obj->advanced;
                g::pOptsManager->pin_advanced_save(obj->get_id(), obj->advanced);
            }
            ImGui::SameLine(0, w::pin_r_offset);
        }

        draw_widget(pin, widget_name_width, widget_value_width);
    }
}

void RoNodeDrawer::draw_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width)
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

void RoNodeDrawer::draw_widget_int(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    int64_t* val = data::Pin::get_int(state->value);
    const int64_t* step = data::Pin::get_int(obj->step);
    const int64_t* min = data::Pin::get_int(obj->min);
    const int64_t* max = data::Pin::get_int(obj->max);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
        ImGuiEx::InputIntSmart("", val, *step, *min, *max, obj->precision, obj->precision);
        break;
    case WidgetInput::Drag:
        ImGuiEx::DragIntSmart("", val, *min, *max, obj->precision.c_str());
        break;
    case WidgetInput::Slider:
    default:
        ImGuiEx::SliderIntSmart("", val, *min, *max, obj->precision.c_str());
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_uint(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    uint64_t* val = data::Pin::get_uint(state->value);
    const uint64_t* step = data::Pin::get_uint(obj->step);
    const uint64_t* min = data::Pin::get_uint(obj->min);
    const uint64_t* max = data::Pin::get_uint(obj->max);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
        ImGuiEx::InputUIntSmart("", val, *step, *min, *max, obj->precision, obj->precision);
        break;
    case WidgetInput::Slider:
    case WidgetInput::Drag:
    default:
        ImGuiEx::DragUIntSmart("", val, *min, *max, obj->precision.c_str());
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_float(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    double* val = data::Pin::get_float(state->value);
    const double* min = data::Pin::get_float(obj->min);
    const double* max = data::Pin::get_float(obj->max);
    const double* step = data::Pin::get_float(obj->step);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
        ImGuiEx::InputDoubleSmart("", val, *step, *min, *max, obj->precision, obj->precision);
        break;
    case WidgetInput::Drag:
        ImGuiEx::DragDoubleSmart("", val, *min, *max, obj->precision.c_str());
        break;
    case WidgetInput::Slider:
    default:
        ImGuiEx::SliderDoubleSmart("", val, *min, *max, obj->precision.c_str());
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;
    bool val = data::Pin::get_bool(state->value);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 0.0f);

    switch (g::pOptsManager->wf_widget_input_bools_load())
    {
    case WidgetInput::Toggle:
        ImGui::SetNextItemWidth(widget_value_width);
        ImGuiEx::BooleanButtons2("", val ? obj->label_on.c_str() : obj->label_off.c_str(), &val);
        break;
    case WidgetInput::Select:
    default:
        ImGuiEx::BooleanToggleLED(val ? obj->label_on.c_str() : obj->label_off.c_str(), &val, false, false, widget_value_width);
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    auto& placeholder = (obj->placeholder.empty() ? obj->name : obj->placeholder);
    std::string* val = data::Pin::get_string(pin->m_state->value);

    ImGui::SetNextItemWidth(widget_name_width + widget_value_width);
    ImGuiEx::InputTextWithHint("", placeholder.c_str(), val, ImGuiInputTextFlags_ReadOnly);

    bool tt_mod = false;
    if (!val->empty())
        tt_mod = ImGuiEx::TooltipOnLastControl(placeholder.c_str(), placeholder.c_str() + placeholder.size(), ImGuiHoveredFlags_DelayNone, true);

    if (!tt_mod)
        ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_str_ml(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    auto& placeholder = (obj->placeholder.empty() ? obj->name : obj->placeholder);
    std::string* val = data::Pin::get_string(pin->m_state->value);

    ImGui::SetNextItemWidth(widget_name_width + widget_value_width);
    ImGuiEx::InputTextMultilineWithHint("", placeholder.c_str(), val, ImVec2(0, 0), ImGuiInputTextFlags_ReadOnly);

    bool tt_mod = false;
    if (!val->empty())
        tt_mod = ImGuiEx::TooltipOnLastControl(placeholder.c_str(), placeholder.c_str() + placeholder.size(), ImGuiHoveredFlags_DelayNone, true);

    if (!tt_mod)
        ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_num(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    draw_widget_int(pin, widget_name_width, widget_value_width);
}

void RoNodeDrawer::draw_widget_combo(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);
    ImGui::Combo("", &pin->m_state->combo_index, obj->combo_items_ptr.data(), cast::to_i32(obj->combo_items_ptr.size()));

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width)
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

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_control_after_generate(data::PinInstance* pin, float widget_name_width, float widget_value_width)
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

            int32_t curr = cast::to_i32(state->control_after_generate_value);
            ImGui::Combo("##CAGCombo", &curr, data::ControlAfterGenerateStr, sizeof(data::ControlAfterGenerateStr) / sizeof(data::ControlAfterGenerateStr[0]));
        }
    }
}

void RoNodeDrawer::draw_widget_separator(float offset, float width)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + offset);
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::NodeWidgetHiddenSep), width, w::hidden_widgets.c_str(), w::hidden_widgets.size(), w::hidden_widgets_len);
}

} // namespace ui
