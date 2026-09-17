#include "ui/node_drawer_ro.h"
#include "core/managers/settings_manager.h"
#include "core/workflow/node.h"
#include "core/workflow/pin.h"
#include "core/workflow/node_instance_ro.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"

namespace ui
{

RoNodeDrawer::RoNodeDrawer(data::Workflow* wf) noexcept
    : m_clr_node_header_bg(g::pOptsManager->color_load(AppColor::NodeHeaderBg))
    , m_clr_node_header_text(g::pOptsManager->color_load(AppColor::NodeHeaderText))
    , m_clr_node_body_bg(g::pOptsManager->color_load(AppColor::NodeBodyBg))
    , m_clr_node_pin_text(g::pOptsManager->color_load(AppColor::NodePinText))
    , m_clr_node_border(g::pOptsManager->color_load(AppColor::NodeBorder))
    , m_wf(wf)
{
}

RoNodeDrawer::~RoNodeDrawer() noexcept = default;

RoNodeDrawer::RoNodeDrawer(RoNodeDrawer&& other) noexcept
    : RoNodeDrawer(other.m_wf)
{
    *this = std::move(other);
}

RoNodeDrawer& RoNodeDrawer::operator=(RoNodeDrawer&& other) noexcept
{
    if (this == &other)
        return *this;

    m_node = other.m_node;
    m_tt_mode = other.m_tt_mode;
    m_curr_node_left_offset = other.m_curr_node_left_offset;
    m_wf = other.m_wf;
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
        if (!pin->get_object()->is_widget())
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
    //const auto out_pins_pos = ImGui::GetCursorPosX();

    const auto& output = m_node->get_outputs();
    for (auto pin : output)
    {
        ImGui::PushID(pin);
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro + w::ico_max + w::pin_r_offset + m_node->w_pin_in + m_node->w_center +
            m_node->w_pin_out - pin->get_object()->def_state.w_name);
        draw_output_pin(pin);
        ImGui::PopID(); // pin id
    }
    ImGui::EndGroup();              // 4 -> out pins

    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(m_node->w_node, w::v_spacing * 2));

    if (m_node->flags().has(NodeInstFlags::HasWidgets))
    {
        // --- WIDGETS ---

        ImGui::BeginGroup();            // 5 -> widgets

        size_t advanced = 0;
        for (auto pin : input)
        {
            auto obj = pin->get_object();
            if (obj->flags.has(PinObjFlags::Advanced))
                advanced++;
            else if (obj->is_widget())
            {
                ImGui::PushID(pin);
                draw_input_widget(pin, m_node->w_widget_name, m_node->w_widget_value);
                ImGui::PopID(); // widget id
            }
        }
        if (advanced > 0)
        {
            draw_widget_separator(w::pin_l_offset_ro + w::ico_max + w::pin_r_offset, m_node->w_separator);
            for (auto pin : input)
            {
                auto obj = pin->get_object();
                if (obj->flags.has(PinObjFlags::Advanced))
                {
                    ImGui::PushID(pin);
                    draw_input_widget(pin, m_node->w_widget_name, m_node->w_widget_value);
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

void RoNodeDrawer::draw_node_header(const data::NodeInstanceRo* node) noexcept
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + node->w_node_name_off);
    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_header_text);
    ImGui::TextUnformatted(node->get_name().c_str(), node->get_name().c_str() + node->get_name().size());
    ImGui::PopStyleColor();
}

void RoNodeDrawer::draw_input_pin(data::PinInstance* pin) noexcept
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro);
    draw_pin_icon(pin->state().pin_color);
    ImGui::SameLine(0.0f, w::pin_r_offset);
    ImGui::TextUnformatted(pin->state().name.c_str(), pin->state().name.c_str() + pin->state().name.size());
}

void RoNodeDrawer::draw_output_pin(data::PinInstance* pin) noexcept
{
    ImGui::TextUnformatted(pin->state().name.c_str(), pin->state().name.c_str() + pin->state().name.size());
    ImGui::SameLine(0, w::pin_r_offset);
    draw_pin_icon(pin->state().pin_color);
}

void RoNodeDrawer::draw_pin_icon(ImU32 color) noexcept
{
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(ICON_LC_CIRCLE_SMALL);
    ImGui::PopStyleColor();

    //const auto cursorPos = ImGui::GetCursorScreenPos();
    //const ImVec2 center = ImVec2(cursorPos.x + mngr::SettingsManager::wf_get_pin_radius(), cursorPos.y + ImGui::GetTextLineHeight() / 2);

    //const auto draw_list = ImGui::GetWindowDrawList();
    //draw_list->AddCircle(center, mngr::SettingsManager::wf_get_pin_radius(), color, 0, 1.0f);
}

void RoNodeDrawer::draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    if (m_tt_mode || pin->obj_flags().has(PinObjFlags::CagPin))
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro + w::ico_max + w::pin_r_offset);
    }
    else
    {
        auto obj = pin->get_object();

        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro);
        ImGui::TextUnformatted(obj->flags.has(PinObjFlags::Advanced) ? ICON_LC_EYE_CLOSED : ICON_LC_EYE);

        const ImVec2 size = ImGui::GetItemRectSize();
        ImGui::SameLine();
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_ro);
        if (ImGui::InvisibleButton("##pin", size))
        {
            obj->flags.toggle(PinObjFlags::Advanced);
            {
                mngr::PinOptions pin_opt;
                pin_opt.flags.copy_from_by_mask_obj(obj->flags, pin_opt.mask);
                g::pOptsManager->items_pin_save(obj->get_hash(), pin_opt);
            }

            if (obj->flags.has(PinObjFlags::CagRequire))
            {
                const auto cag = m_wf->get_pin_by_instance(pin->state().cag);
                cag->obj_flags().add_remove_if(PinObjFlags::Advanced, obj->flags.has(PinObjFlags::Advanced));
                {
                    mngr::PinOptions pin_opt;
                    pin_opt.flags.copy_from_by_mask_obj(cag->obj_flags(), pin_opt.mask);
                    g::pOptsManager->items_pin_save(cag->get_object()->get_hash(), pin_opt);
                }
            }
        }
        ImGui::SameLine(0, w::pin_r_offset);
    }

    draw_widget(pin, widget_name_width, widget_value_width);
}

void RoNodeDrawer::draw_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    switch (pin->state().type)
    {
    case PinType::INT:
        draw_widget_int(pin, widget_name_width, widget_value_width);
        break;
    case PinType::FLOAT:
        draw_widget_float(pin, widget_name_width, widget_value_width);
        break;
    case PinType::NUMBER:
        draw_widget_number(pin, widget_name_width, widget_value_width);
        break;
    case PinType::BOOLEAN:
        draw_widget_bool(pin, widget_name_width, widget_value_width);
        break;
    case PinType::STRING:
        if (pin->obj_flags().has(PinObjFlags::Multiline))
            draw_widget_str_ml(pin, widget_name_width, widget_value_width);
        else
            draw_widget_str(pin, widget_name_width, widget_value_width);
        break;
    case PinType::COMBO:
        draw_widget_combo(pin, widget_name_width, widget_value_width);
        break;
    case PinType::COLOR:
        draw_widget_color(pin, widget_name_width, widget_value_width);
        break;
    }
}

void RoNodeDrawer::draw_widget_int(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    int64_t* val = state.value.get_int_ref();
    LogRAssert(val, "Zero ptr", );

    const int64_t step = obj->step.get_int();
    const int64_t min = obj->min.get_int();
    const int64_t max = obj->max.get_int();

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
        ImGuiEx::InputIntSmart("", val, step, min, max, obj->precision, obj->precision);
        break;
    case WidgetInput::Drag:
        ImGuiEx::DragIntSmart("", val, min, max, obj->precision.c_str());
        break;
    case WidgetInput::Slider:
    default:
        ImGuiEx::SliderIntSmart("", val, min, max, obj->precision.c_str());
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_float(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    double* val = state.value.get_float_ref();
    LogRAssert(val, "Zero ptr", );

    const double min = obj->min.get_float();
    const double max = obj->max.get_float();
    const double step = obj->step.get_float();

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
        ImGuiEx::InputDoubleSmart("", val, step, min, max, obj->precision, obj->precision);
        break;
    case WidgetInput::Drag:
        ImGuiEx::DragDoubleSmart("", val, min, max, obj->precision.c_str());
        break;
    case WidgetInput::Slider:
    default:
        ImGuiEx::SliderDoubleSmart("", val, min, max, obj->precision.c_str());
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_number(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    static_assert(std::same_as<number_t, double>, "Update this function if number_t changes");

    number* num_ptr = state.value.get_number_ref();
    LogRAssert(num_ptr, "Zero ptr", );

    double* val = num_ptr->underlying_type_ref();
    LogRAssert(val, "Zero ptr", );

    const double min = obj->min.get_number().underlying_type();
    const double max = obj->max.get_number().underlying_type();
    const double step = obj->step.get_number().underlying_type();

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
        ImGuiEx::InputDoubleSmart("", val, step, min, max, obj->precision, obj->precision);
        break;
    case WidgetInput::Drag:
        ImGuiEx::DragDoubleSmart("", val, min, max, obj->precision.c_str());
        break;
    case WidgetInput::Slider:
    default:
        ImGuiEx::SliderDoubleSmart("", val, min, max, obj->precision.c_str());
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();
    auto val = state.value.get_bool_ref();
    LogRAssert(val, "Zero ptr", );

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 0.0f);

    switch (g::pOptsManager->wf_widget_input_bools_load())
    {
    case WidgetInput::Toggle:
        ImGui::SetNextItemWidth(widget_value_width);
        ImGuiEx::BooleanButtons2("", *val ? obj->label_on.c_str() : obj->label_off.c_str(), val);
        break;
    case WidgetInput::Select:
    default:
        ImGuiEx::BooleanToggleLED(*val ? obj->label_on.c_str() : obj->label_off.c_str(), val, false, false, widget_value_width);
        break;
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    const auto& placeholder = (obj->placeholder.empty() ? pin->state().name : obj->placeholder);
    std::string* val = pin->state().value.get_string_ref();
    LogRAssert(val, "Zero ptr", );

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

    const auto& placeholder = (obj->placeholder.empty() ? pin->state().name : obj->placeholder);
    std::string* val = pin->state().value.get_string_ref();
    LogRAssert(val, "Zero ptr", );

    ImGui::SetNextItemWidth(widget_name_width + widget_value_width);
    ImGuiEx::InputTextMultilineWithHint("", placeholder.c_str(), val, ImVec2(0, 0), ImGuiInputTextFlags_ReadOnly);

    bool tt_mod = false;
    if (!val->empty())
        tt_mod = ImGuiEx::TooltipOnLastControl(placeholder.c_str(), placeholder.c_str() + placeholder.size(), ImGuiHoveredFlags_DelayNone, true);

    if (!tt_mod)
        ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_combo(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);
    int32_t index = cast::to_i32(pin->state().value.get_int());
    ImGui::Combo("", &index, obj->combo_items_ptr.data(), cast::to_i32(obj->combo_items_ptr.size()));

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const ImVec4 vec4 = ImGui::ColorConvertU32ToFloat4(cast::to_u32(state.value.get_int()));
    float colors[4];
    vec4.to_arr(colors);
    if (ImGui::ColorEdit4("", colors,
        ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop |
        ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoTooltip | (obj->flags.has(PinObjFlags::ClrAlphaEnabled) ? 0 : ImGuiColorEditFlags_NoAlpha)))
    {
        state.value = data::Pin::colors_to_int(ImVec4(colors));
    }

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
}

void RoNodeDrawer::draw_widget_separator(float offset, float width) noexcept
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + offset);
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::NodeWidgetHiddenSep), width, "Hidden widgets");
}

} // namespace ui
