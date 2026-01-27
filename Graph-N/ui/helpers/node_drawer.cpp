
#include "ui/helpers/node_drawer.h"
#include "ui/helpers/imgui_ex_ne.h"
#include "core/managers/settings_manager.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "core/workflow/link.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "imgui_node_editor.h"

namespace ui
{

namespace ed = ax::NodeEditor;

NodeDrawer::NodeDrawer()
    : m_clr_node_header_bg(g::pOptsManager->color_load(AppColor::NodeHeaderBg))
    , m_clr_node_header_text(g::pOptsManager->color_load(AppColor::NodeHeaderText))
    , m_clr_node_body_bg(g::pOptsManager->color_load(AppColor::NodeBodyBg))
    , m_clr_node_pin_text(g::pOptsManager->color_load(AppColor::NodePinText))
    , m_clr_node_border(g::pOptsManager->color_load(AppColor::NodeBorder))
{
    LogF;
}

NodeDrawer::~NodeDrawer()
{
    LogF;

    if (m_context != nullptr)
        ed::DestroyEditor(m_context);
}

void test_id(std::set<void*>& set, void* ptr)
{
    auto res = set.insert(ptr);
    if (!res.second)
    {
        DEBUG_CATCH;
    }
}

void NodeDrawer::init()
{
    LogF;

    m_deffered_action = std::make_unique<ImGuiExNe::DeferredAction>();

    ed::Config config;
    config.SettingsFile = nullptr;
    m_context = ed::CreateEditor(&config);
}

void NodeDrawer::free()
{
    LogF;

    m_deffered_action.reset();

    if (m_context != nullptr)
        ed::DestroyEditor(m_context);
    m_context = nullptr;
}

void NodeDrawer::pre_draw_all(const char* id)
{
    ed::SetCurrentEditor(m_context);
    ed::Begin(id, ImVec2(0, 0));
}

void NodeDrawer::post_draw_all()
{
    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGuiExNe::RenderDeferredCombo(*m_deffered_action.get());
}

void NodeDrawer::pre_draw_node(data::NodeInstance* node)
{
    const bool deferred_node_active = m_deffered_action->active_node == node;
    if (deferred_node_active)
    {
        if (!ed::IsNodeSelected(cast::to_ptr(node)))
            ed::SelectNode(cast::to_ptr(node), true);
    }
    else if (m_deffered_action->active_node != nullptr)
    {
        ed::DeselectNode(cast::to_ptr(node));
    }

    ed::BeginNode(cast::to_ptr(node));
}

void NodeDrawer::post_draw_node(data::NodeInstance* /*node*/)
{
    ed::EndNode();
}

data::PinInstance* NodeDrawer::get_ml_widget_active() const
{
    if (nullptr == m_deffered_action->active_pin)
        return nullptr;
    return cast::ptrs<data::PinInstance>(m_deffered_action->active_pin);
}

void NodeDrawer::draw_node(data::NodeInstance* node, float ideal_fit_w)
{
    m_ids_imgui.clear();
    m_ids_nodelib.clear();
    m_ids_imgui.insert(node);
    m_ids_nodelib.insert(node);

    m_active_node = node;

    const ImVec2 node_size = ed::GetNodeSize(node->get_graph_id());
    if (node_size.x > 0 && node_size.y > 0)
        ideal_fit_w = node_size.x - ed::GetStyle().NodePadding.x - ed::GetStyle().NodePadding.z;

    ideal_fit_w = node->recalc_node_width(false /*ro*/, ideal_fit_w, true /*allow_extend*/, true /*allow_compress*/, false /*use_big_pin_icon*/);
    m_curr_node_left_offset = ImGui::GetCursorPosX();

    // --- HEADER ---

    ImGui::Dummy(ImVec2(node->w_node, w::v_spacing));
    draw_node_header(node);
    ImGui::Dummy(ImVec2(node->w_node, w::v_spacing));

    // --- PINS - INPUT ---

    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_pin_text);

    const auto& input = node->m_input;
    for (auto pin : input)
    {
        if (nullptr == pin || !pin->is_valid())
        {
            LogError(ExcepCategory::Logic, "Null pin");
        }
        else if (!pin->get_object()->is_widget())
        {
            ImGui::PushID(pin);
            test_id(m_ids_imgui, pin);
            draw_input_pin(pin);
            ImGui::PopID();
        }
    }

    // --- SPACER ---

    if (!node->m_output.empty())
    {
        ImGui::SameLine(0.0f, 0.0f);
        if (node->w_center > 0.0f)
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::ico_max + w::pin_r_offset + node->w_pin_in + node->w_center);
    }

    // --- PINS - OUTPUT ---

    const auto out_pins_pos = ImGui::GetCursorPosX();

    const auto& output = node->m_output;
    for (auto pin : output)
    {
        if (nullptr == pin || !pin->is_valid())
        {
            LogError(ExcepCategory::Logic, "Null pin");
        }
        else
        {
            ImGui::PushID(pin);
            test_id(m_ids_imgui, pin);
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::ico_max + w::pin_r_offset + node->w_pin_in + node->w_center +
                node->w_pin_out - pin->get_object()->w_name);
            draw_output_pin(pin);
            ImGui::PopID();
        }
    }

    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(node->w_node, w::v_spacing * 2));

    if (node->m_has_widgets)
    {
        // --- WIDGETS ---

        size_t advanced = 0;
        for (auto pin : input)
        {
            auto obj = pin->get_object();
            if (obj->advanced)
                advanced++;
            else if (obj->is_widget())
            {
                ImGui::PushID(pin);
                test_id(m_ids_imgui, pin);
                draw_input_widget(pin, node->w_widget_name, node->w_widget_value, false);

                if (obj->control_after_generate)
                {
                    ImGui::PushID(&pin->m_state->control_after_generate_value);
                    test_id(m_ids_imgui, &pin->m_state->control_after_generate_value);
                    draw_input_widget(pin, node->w_widget_name, node->w_widget_value, true);
                    ImGui::PopID();
                }
                ImGui::PopID();
            }
        }
        if (advanced > 0)
        {
            draw_widget_separator(w::pin_l_offset_rw + w::ico_max + w::pin_r_offset, node->w_separator);
            for (auto pin : input)
            {
                auto obj = pin->get_object();
                if (obj->advanced)
                {
                    ImGui::PushID(pin);
                    test_id(m_ids_imgui, pin);
                    draw_input_widget(pin, node->w_widget_name, node->w_widget_value, false);

                    if (obj->control_after_generate)
                    {
                        ImGui::PushID(&pin->m_state->control_after_generate_value);
                        test_id(m_ids_imgui, &pin->m_state->control_after_generate_value);
                        draw_input_widget(pin, node->w_widget_name, node->w_widget_value, true);
                        ImGui::PopID();
                    }
                    ImGui::PopID();
                }
            }
        }

        ImGui::Dummy(ImVec2(node->w_node, 10.0f));
    }

    m_active_node = nullptr;
    //return ideal_fit_w;
}

void NodeDrawer::draw_node_header(data::NodeInstance* node)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + node->w_node_name_off);
    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_header_text);
    ImGui::TextUnformatted(node->get_name().c_str(), node->get_name().c_str() + node->get_name().size());
    ImGui::PopStyleColor();
}

void NodeDrawer::draw_input_pin(data::PinInstance* pin)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw);
    draw_pin(pin);
    ImGui::SameLine(0.0f, w::pin_r_offset);
    ImGui::TextUnformatted(pin->get_object()->name.c_str(), pin->get_object()->name.c_str() + pin->get_object()->name.size());
}

void NodeDrawer::draw_output_pin(data::PinInstance* pin)
{
    ImGui::TextUnformatted(pin->get_object()->name.c_str(), pin->get_object()->name.c_str() + pin->get_object()->name.size());
    ImGui::SameLine(0, w::pin_r_offset);
    draw_pin(pin);
}

void NodeDrawer::draw_pin(data::PinInstance* pin)
{
    //ImGui::PushStyleColor(ImGuiCol_Text, pin_color);
    ImGui::PushID(&pin->m_state->pin_color);
    test_id(m_ids_imgui, &pin->m_state->pin_color);
    ed::BeginPin(cast::to_ptr(pin), pin->get_object()->input ? ed::PinKind::Input : ed::PinKind::Output);
    test_id(m_ids_nodelib, pin);
    //ImGui::TextUnformatted(ICON_LC_CIRCLE_SMALL);
    draw_pin_shape(pin);
    ed::EndPin();
    ImGui::PopID();
 //   ImGui::PopStyleColor();
}

void NodeDrawer::draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width, bool draw_control_after_generate /*= false*/)
{
    if (draw_control_after_generate)
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::ico_max + w::pin_r_offset);
        draw_widget_control_after_generate(pin, widget_name_width, widget_value_width);
    }
    else
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw);
        draw_pin(pin);
        ImGui::SameLine(0, w::pin_r_offset);
        draw_widget(pin, widget_name_width, widget_value_width);
    }
}

void NodeDrawer::draw_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width)
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

void NodeDrawer::handle_widget_active_state()
{
    if (ImGui::IsItemActivated())
    {
        ed::SelectNode(cast::to_ptr(m_active_node));
    }
}

void NodeDrawer::draw_widget_int(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    int64_t* val = data::Pin::get_int(state->value);
    const int64_t* step = data::Pin::get_int(obj->step);
    const int64_t* min = data::Pin::get_int(obj->min);
    const int64_t* max = data::Pin::get_int(obj->max);

    ImGui::PushID(val);
    test_id(m_ids_imgui, val);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
        ImGuiEx::InputIntSmart("##value", val, *step, *min, *max, obj->precision, obj->precision);
        break;
    case WidgetInput::Drag:
        ImGuiEx::DragIntSmart("##value", val, *min, *max, obj->precision.c_str());
        break;
    case WidgetInput::Slider:
    default:
        ImGuiEx::SliderIntSmart("##value", val, *min, *max, obj->precision.c_str());
        break;
    }
    handle_widget_active_state();

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_uint(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    uint64_t* val = data::Pin::get_uint(state->value);
    const uint64_t* step = data::Pin::get_uint(obj->step);
    const uint64_t* min = data::Pin::get_uint(obj->min);
    const uint64_t* max = data::Pin::get_uint(obj->max);

    ImGui::PushID(val);
    test_id(m_ids_imgui, val);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
        ImGuiEx::InputUIntSmart("##value", val, *step, *min, *max, obj->precision, obj->precision);
        break;
    case WidgetInput::Slider:
    case WidgetInput::Drag:
    default:
        ImGuiEx::DragUIntSmart("##value", val, *min, *max, obj->precision.c_str());
        break;
    }
    handle_widget_active_state();

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_float(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    double* val = data::Pin::get_float(state->value);
    const double* min = data::Pin::get_float(obj->min);
    const double* max = data::Pin::get_float(obj->max);
    const double* step = data::Pin::get_float(obj->step);

    ImGui::PushID(val);
    test_id(m_ids_imgui, val);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
        ImGuiEx::InputDoubleSmart("##value", val, *step, *min, *max, obj->precision, obj->precision);
        break;
    case WidgetInput::Drag:
        ImGuiEx::DragDoubleSmart("##value", val, *min, *max, obj->precision.c_str());
        break;
    case WidgetInput::Slider:
    default:
        ImGuiEx::SliderDoubleSmart("##value", val, *min, *max, obj->precision.c_str());
        break;
    }
    handle_widget_active_state();

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;
    bool* val = data::Pin::get_bool(state->value);

    ImGui::PushID(val);
    test_id(m_ids_imgui, val);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 0.0f);

    switch (g::pOptsManager->wf_widget_input_bools_load())
    {
    case WidgetInput::Toggle:
        ImGui::SetNextItemWidth(widget_value_width);
        ImGuiEx::BooleanButtons2("##value", *val ? obj->label_on.c_str() : obj->label_off.c_str(), val);
        break;
    case WidgetInput::Select:
    default:
        ImGuiEx::BooleanToggleLED(*val ? obj->label_on.c_str() : obj->label_off.c_str(), val, false, false, widget_value_width);
        break;
    }
    handle_widget_active_state();

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    auto& placeholder = (obj->placeholder.empty() ? obj->name : obj->placeholder);
    std::string* val = data::Pin::get_string(pin->m_state->value);

    ImGui::PushID(val);
    test_id(m_ids_imgui, val);

    ImGui::PushItemWidth(widget_name_width + widget_value_width);
    ImGuiEx::InputTextWithHint("##value", placeholder.c_str(), val);
    handle_widget_active_state();
    ImGui::PopItemWidth();

    bool tt_mod = false;
    if (!val->empty())
        tt_mod = ImGuiEx::TooltipOnLastControl(placeholder.c_str(), placeholder.c_str() + placeholder.size(), ImGuiHoveredFlags_DelayNone, true);

    if (!tt_mod)
        ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());

    ImGui::PopID();
}

void NodeDrawer::draw_widget_str_ml(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    auto& placeholder = (obj->placeholder.empty() ? obj->name : obj->placeholder);
    std::string* val = data::Pin::get_string(pin->m_state->value);

    ImGui::PushID(val);
    test_id(m_ids_imgui, val);

    ImGui::PushTextWrapPos(0.0f);
    ImGui::SetNextItemWidth(widget_name_width + widget_value_width);
    ImGuiExNe::InputTextWithHint(*m_deffered_action.get(), m_active_node, pin, "##value", val->c_str(), placeholder.c_str());
    ImGui::PopTextWrapPos();

    bool tt_mod = false;
    if (!val->empty())
        tt_mod = ImGuiEx::TooltipOnLastControl(placeholder.c_str(), placeholder.c_str() + placeholder.size(), ImGuiHoveredFlags_DelayNone, true);

    if (!tt_mod)
        ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());

    ImGui::PopID();
}

void NodeDrawer::draw_widget_num(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    draw_widget_int(pin, widget_name_width, widget_value_width);
}

void NodeDrawer::draw_widget_combo(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    ImGui::PushID(&pin->m_state->combo_index);
    test_id(m_ids_imgui, &pin->m_state->combo_index);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    ImGuiExNe::Combo(*m_deffered_action.get(), m_active_node, pin, "##value", &pin->m_state->combo_index, obj->combo_items_ptr.data(), cast::to_i32(obj->combo_items_ptr.size()));
    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    ImGui::PushID(state->colors);
    test_id(m_ids_imgui, state->colors);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    auto colors = state->colors;
    if (ImGui::ColorEdit4("##value", colors,
        ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop |
        ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoTooltip | (obj->alpha_enabled ? 0 : ImGuiColorEditFlags_NoAlpha)))
    {
        state->value = data::Pin::colors_to_int(state->colors);
    }
    handle_widget_active_state();

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_control_after_generate(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    if (obj->control_after_generate)
    {
        auto& state = pin->m_state;
        if (data::PinType::INT == state->type || data::PinType::FLOAT == state->type || data::PinType::NUMBER == state->type)
        {
            ImGui::PushID(&state->combo_index);
            test_id(m_ids_imgui, &state->combo_index);

            ImGui::SetNextItemWidth(widget_name_width);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::InputText("##CAGControlName", "control", ImGuiInputTextFlags_ReadOnly);
            handle_widget_active_state();
            ImGui::PopStyleColor();

            ImGui::SameLine(0.0f, 0.0f);
            ImGui::SetNextItemWidth(widget_value_width);
            ImGuiExNe::Combo(*m_deffered_action.get(), m_active_node, pin, "##CAGComboValue", &state->control_after_generate_value, data::ControlAfterGenerateStr, sizeof(data::ControlAfterGenerateStr) / sizeof(data::ControlAfterGenerateStr[0]));
            ImGui::PopID();
        }
    }
}

void NodeDrawer::draw_widget_separator(float offset, float width)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + offset);
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::NodeWidgetHiddenSep), width, w::hidden_widgets.c_str(), w::hidden_widgets.size(), w::hidden_widgets_len);
}

void NodeDrawer::draw_links(const std::vector<std::unique_ptr<data::Link>>& links)
{
    for (auto& link : links)
    {
        test_id(m_ids_nodelib, link.get());
        auto in = link->get_input();
        auto out = link->get_output();
        ed::Link(cast::to_ptr(link.get()), cast::to_ptr(in), cast::to_ptr(out), in->m_state->pin_color);
    }
}

void NodeDrawer::query_links()
{
    if (ed::BeginCreate())
    {
        ed::PinId start = 0, end = 0;

        if (ed::QueryNewLink(&start, &end))
        {
            data::PinInstance* p1 = cast::ptrs<data::PinInstance>(start.Get());
            data::PinInstance* p2 = cast::ptrs<data::PinInstance>(end.Get());
            if (data::Link::can_link(p1, p2))
            {
                if (ed::AcceptNewItem())
                {
                    m_querying_pin = nullptr;
                    data::Link::link(p1, p2);
                }
            }
            else
            {
                ed::RejectNewItem();
            }
        }
        else if (ed::QueryNewNode(&start))
        {
            m_querying_pin = cast::ptrs<data::PinInstance>(start.Get());
        }
        ed::EndCreate();
    }
    else
    {
        m_querying_pin = nullptr;
    }

    static data::PinInstance* test = nullptr;
    if (test != m_querying_pin)
    {
        test = m_querying_pin;
        if (test != nullptr)
            ::OutputDebugStringA(fmt::format("Drag pin {}", test->get_object()->name).c_str());
        else
            ::OutputDebugStringA("Drag pin is 0");
    }
}

void NodeDrawer::draw_pin_shape(data::PinInstance* pin)
{
    const bool querying = (m_querying_pin != nullptr);
    const bool compatible = (m_querying_pin == pin || (querying && data::Link::can_link(pin, m_querying_pin)));
    const bool connected = (m_querying_pin == pin || pin->m_state->links > 0);
    const auto type = data::Pin::get_shape(pin->m_state->type);
    constexpr const float thickness = 2.0f;
    constexpr const int32_t segments = 12;
    const auto draw_list = ImGui::GetWindowDrawList();
    const float alpha = (!querying || compatible ? 1.0f : 0.4f);
    const auto color = (pin->m_state->pin_color & 0x00FFFFFF) | ((ImU32)(alpha * 255.0f) << 24);

    const float line_height = ImGui::GetTextLineHeight();
    const float radius = (!querying ? 4.0f : (compatible ? 5.0f : 3.0f));
    const float diameter = radius * 2.0f;
    const float y_offset = std::max(0.0f, (line_height - diameter) * 0.5f); 
    const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    const ImVec2 pin_center = ImVec2(cursor_pos.x + w::ico_circle / 2, cursor_pos.y + w::ico_circle / 2);

    switch (type)
    {
    case data::PinShape::Diamond:
        {
            const float r = radius * 1.1f;
            if (connected)
            {
                draw_list->AddQuadFilled(ImVec2(pin_center.x, pin_center.y - r), ImVec2(pin_center.x + r, pin_center.y), ImVec2(pin_center.x, pin_center.y + r), ImVec2(pin_center.x - r, pin_center.y), color);
            }
            else
            {
                const ImVec2 p1(pin_center.x, pin_center.y - r);
                const ImVec2 p2(pin_center.x + r, pin_center.y);
                const ImVec2 p3(pin_center.x, pin_center.y + r);
                const ImVec2 p4(pin_center.x - r, pin_center.y);
                draw_list->PathLineTo(p1);
                draw_list->PathLineTo(p2);
                draw_list->PathLineTo(p3);
                draw_list->PathLineTo(p4);
                draw_list->PathStroke(color, ImDrawFlags_Closed, thickness);
            }
        }
        break;
    case data::PinShape::Hexagon:
        {
            const float r = radius * 0.95f;
            if (connected)
            {
                ImVec2 points[6];
                for (int32_t i = 0; i < 6; ++i)
                {
                    const float angle = cast::to_float(i) * 1.047f;
                    points[i] = ImVec2(pin_center.x + r * cosf(angle), pin_center.y + r * sinf(angle));
                }
                draw_list->AddConvexPolyFilled(points, 6, color);
            }
            else
            {
                draw_list->PathClear();
                for (int32_t i = 0; i < 6; ++i)
                {
                    float angle = cast::to_float(i) * 1.047f;
                    draw_list->PathLineTo(ImVec2(pin_center.x + r * cosf(angle), pin_center.y + r * sinf(angle)));
                }
                draw_list->PathStroke(color, ImDrawFlags_Closed, thickness);
            }
        }
        break;
    case data::PinShape::SemiCircle:
        {
            const bool in = pin->get_object()->input;
            if (in)
                draw_list->PathArcTo(pin_center, radius, IM_PI * 1.5f, IM_PI * 2.5f, segments / 2);
            else
                draw_list->PathArcTo(pin_center, radius, IM_PI * 0.5f, IM_PI * 1.5f, segments / 2);
            draw_list->PathFillConvex(color);
        }
        break;
    case data::PinShape::Square:
        {
            if (connected)
                draw_list->AddRectFilled(pin_center - ImVec2(radius, radius) * 0.85f, pin_center + ImVec2(radius, radius) * 0.85f, color, radius * 0.2f);
            else
                draw_list->AddRect(pin_center - ImVec2(radius, radius) * 0.85f, pin_center + ImVec2(radius, radius) * 0.85f, color, radius * 0.2f, ImDrawFlags_None, thickness);
        }
        break;
    case data::PinShape::Triangle:
        {
            const float r = radius * 1.1f;
            const float angle_offset = 0.0f;
            const ImVec2 p1 = ImVec2(pin_center.x + r * cosf(angle_offset), pin_center.y + r * sinf(angle_offset));
            const ImVec2 p2 = ImVec2(pin_center.x + r * cosf(angle_offset + 2.094f), pin_center.y + r * sinf(angle_offset + 2.094f));
            const ImVec2 p3 = ImVec2(pin_center.x + r * cosf(angle_offset + 4.188f), pin_center.y + r * sinf(angle_offset + 4.188f));
            if (connected)
                draw_list->AddTriangleFilled(p1, p2, p3, color);
            else
                draw_list->AddTriangle(p1, p2, p3, color, thickness);
        }
        break;
    default:
        LogInternalLogic("Unhandled pin shape, pin type " << cast::to_i32(type));
    case data::PinShape::Circle:
        {
            if (connected)
                draw_list->AddCircleFilled(pin_center, radius, color, segments);
            else
                draw_list->AddCircle(pin_center, radius, color, segments, thickness);
            break;
        }
    }

    ImGui::Dummy(ImVec2(w::ico_circle, w::ico_circle));
}

} // namespace ui
