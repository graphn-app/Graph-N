
#include "ui/helpers/node_drawer.h"
#include "ui/helpers/imgui_ex_ne.h"
#include "main.h"
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
    : m_clr_node_header_text(g::pOptsManager->color_load(AppColor::NodeHeaderText))
    , m_clr_node_pin_text(g::pOptsManager->color_load(AppColor::NodePinText))
    , m_menuid_canvas("533177A6-9C98-475D-A152-8F68F4F2353E")
    , m_menuid_node("C0CFBFD5-658A-4E06-9FBC-A3676228C4D2")
    , m_menuid_pin("0398C2D2-1ABE-4C0B-92D3-9B41BBAF4867")
    , m_menuid_link("EF4B02B1-C1ED-4632-A622-C9EAC4A16E8C")
{
    LogF;
}

NodeDrawer::~NodeDrawer()
{
    LogF;

    if (m_context != nullptr)
        ed::DestroyEditor(m_context);
}

#ifdef _DEBUG
void test_id(std::set<void*>& set, void* ptr)
{
    auto res = set.insert(ptr);
    if (!res.second)
    {
        DEBUG_CATCH;
    }
}
#endif // #ifdef _DEBUG

void NodeDrawer::init(wid wf_id, const Callbacks& callbacks)
{
    LogF;

    m_wf = wf_id;
    m_callbacks = callbacks;
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

void NodeDrawer::pre_draw_all(const char* id, size_t nodes_count)
{
    m_resizing_grip_positions.clear();
    m_resizing_grip_positions.resize(nodes_count);

    m_active_tt = nullptr;

    if (m_resizing_w.size() != nodes_count)
    {
        m_now_resizing = -1;
        m_resizing_w.clear();
        m_resizing_w.resize(nodes_count);
    }

    ed::EditConfig(m_context).AlignPointToGrid = g::pOptsManager->wf_snap_nodes_load();
    ed::EditConfig(m_context).DrawGrid = g::pOptsManager->wf_show_grid_load();
    ed::SetCurrentEditor(m_context);
    ed::EnableShortcuts(false);

    m_clr_node_bg = ImGui::ColorConvertFloat4ToU32(ed::GetStyle().Colors[ed::StyleColor_NodeBg]);
    m_clr_node_header_bg = ImGuiEx::decrease_brightness(m_clr_node_bg);

    ed::Begin(id, ImVec2(0, 0));
}

void NodeDrawer::post_draw_all()
{
    ed::Suspend();
    bool canvas_busy = handle_context_menu();
    if (!canvas_busy)
        canvas_busy = handle_node_resize();
    ed::Resume();

    ed::End();

    if (m_menuid_activate != nullptr)
    {
        ImGui::OpenPopup(m_menuid_activate);
        m_menuid_activate = nullptr;
    }
    if (handle_canvas_menu() || handle_node_menu() || handle_pin_menu() || handle_link_menu())
        ;
    else if (!canvas_busy && m_deffered_action->combo.is_active)
        ImGuiExNe::RenderDeferredCombo(*m_deffered_action.get());
    else if (!canvas_busy && m_deffered_action->ml_input.is_active)
    {
        g::pFlow->txt_panel_create(
            m_wf,
            cast::ptrs<data::NodeInstance>(m_deffered_action->active_node),
            cast::ptrs<data::PinInstance>(m_deffered_action->active_pin),
            m_deffered_action->ml_input.buffer,
            m_deffered_action->ml_input.hint);
        m_deffered_action->ml_input.is_active = false;
    }

    if (m_active_tt != nullptr)
        ImGuiEx::TooltipOnLastControl(m_active_tt, nullptr);

    ed::SetCurrentEditor(nullptr);
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

    if (node->m_bg_color_custom)
        ed::PushStyleColor(ed::StyleColor_NodeBg, node->m_bg_color);
    if (node->get_mode() == NodeMode::Mute || node->get_mode() == NodeMode::OnEvent || node->get_mode() == NodeMode::OnTrigger)
    {
        ed::PushStyleColor(ed::StyleColor_NodeBorder, ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::NodeBorderMuted)));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * m_mode_alpha);
    }
    else if (node->get_mode() == NodeMode::Bypass)
    {
        ed::PushStyleColor(ed::StyleColor_NodeBorder, ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::NodeBorderBypassed)));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * m_mode_alpha);
    }

    ed::BeginNode(cast::to_ptr(node));

    if (!node->m_pos_set)
    {
        ed::SetNodePosition(cast::to_ptr(node), node->m_pos);
        node->m_pos_set = true;
    }
}

void NodeDrawer::post_draw_node(data::NodeInstance* node)
{
    ed::EndNode();
    node->m_pos = ed::GetNodePosition(cast::to_ptr(node));

    if (node->m_bg_color_custom)
        ed::PopStyleColor();
    if (node->get_mode() != NodeMode::Always)
    {
        ed::PopStyleColor();
        ImGui::PopStyleVar();
    }

    draw_node_header_bg(node);
}

void NodeDrawer::locate_node(data::NodeInstance* node)
{
    ed::ClearSelection();
    ed::SelectNode(cast::to_ptr(node));
    ed::NavigateToSelection(false, 0.5f);
}

data::PinInstance* NodeDrawer::get_ml_widget_active() const
{
    if (nullptr == m_deffered_action->active_pin)
        return nullptr;
    return cast::ptrs<data::PinInstance>(m_deffered_action->active_pin);
}

void NodeDrawer::draw_node(data::NodeInstance* node, float node_w, size_t node_idx)
{
#ifdef _DEBUG
    m_ids_imgui.clear();
    m_ids_nodelib.clear();
    m_ids_imgui.insert(node);
    m_ids_nodelib.insert(node);
#endif

    m_active_node = node;

    ImVec2 node_size, node_pos;

    if (m_now_resizing == node_idx)
        node_w = m_resizing_w[node_idx];
    else
    {
        node_size = ed::GetNodeSize(cast::to_ptr(node));
        if (node_size.x > 0 && node_size.y > 0)
        {
            node_pos = ed::GetNodePosition(cast::to_ptr(node));
            node_w = node_size.x - ed::GetStyle().NodePadding.x - ed::GetStyle().NodePadding.z;
        }
    }

    node_w = node->recalc_node_width(false /*ro*/, node_w, true /*allow_extend*/, true /*allow_compress*/, false /*use_big_pin_icon*/);

    if (node_size.x > 0 && node_size.y > 0)
    {
        const ImVec2 br = ed::CanvasToScreen(ImVec2(node_pos.x + node_w, node_pos.y + node_size.y));

        m_resizing_w[node_idx] = (node->pinned() || !node->expanded() ? -1 : node_w);
        m_resizing_grip_positions[node_idx] = { br.x - m_grip_area_sz, br.y - m_grip_area_sz, br.x + m_grip_area_sz, br.y + m_grip_area_sz };
    }

    m_curr_node_left_offset = ImGui::GetCursorPosX();
    m_curr_node_top_offset = ImGui::GetCursorPosY();

    // --- HEADER ---

    //ImGui::Dummy(ImVec2(node->w_node, w::v_spacing));
    draw_node_header(node);
    ImGui::Dummy(ImVec2(node->w_node, node->expanded() ? ImGui::GetTextLineHeight() * 0.5f : 0.0f));


    const auto pin_y = ImGui::GetCursorPosY();
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
#ifdef _DEBUG
            test_id(m_ids_imgui, pin);
#endif
            draw_input_pin(pin);
            ImGui::PopID();
        }
    }

    // --- SPACER ---

    if (!node->m_output.empty())
    {
        ImGui::SetCursorPosY(pin_y);
        if (node->expanded() && node->w_center > 0.0f)
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
#ifdef _DEBUG
            test_id(m_ids_imgui, pin);
#endif
            if (node->expanded())
            {
                ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::ico_max + w::pin_r_offset + node->w_pin_in + node->w_center +
                    node->w_pin_out - pin->get_object()->w_name);
            }
            else
            {
                ImGui::SameLine();
                ImGui::SetCursorPosX(m_curr_node_left_offset + node_w - w::pin_r_offset);
            }
            draw_output_pin(pin);
            ImGui::PopID();
        }
    }

    ImGui::PopStyleColor();

    if (node->expanded())
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
#ifdef _DEBUG
                test_id(m_ids_imgui, pin);
#endif
                draw_input_widget(pin, node->w_widget_name, node->w_widget_value, false);

                if (obj->control_after_generate)
                {
                    ImGui::PushID(&pin->m_state->control_after_generate_value);
#ifdef _DEBUG
                    test_id(m_ids_imgui, &pin->m_state->control_after_generate_value);
#endif
                    draw_input_widget(pin, node->w_widget_name, node->w_widget_value, true);
                    ImGui::PopID();
                }
                ImGui::PopID();
            }
        }
        if (advanced > 0 && node->expanded())
        {
            bool temp_open = false;
            draw_widget_separator(w::pin_l_offset_rw + w::ico_max + w::pin_r_offset, node->m_show_hidden, temp_open);
            if (node->m_show_hidden || temp_open)
            {
                for (auto pin : input)
                {
                    auto obj = pin->get_object();
                    if (obj->advanced)
                    {
                        ImGui::PushID(pin);
#ifdef _DEBUG
                        test_id(m_ids_imgui, pin);
#endif
                        draw_input_widget(pin, node->w_widget_name, node->w_widget_value, false);

                        if (obj->control_after_generate)
                        {
                            ImGui::PushID(&pin->m_state->control_after_generate_value);
#ifdef _DEBUG
                            test_id(m_ids_imgui, &pin->m_state->control_after_generate_value);
#endif
                            draw_input_widget(pin, node->w_widget_name, node->w_widget_value, true);
                            ImGui::PopID();
                        }
                        ImGui::PopID();
                    }
                }
            }
        }
        else
            node->m_show_hidden = false;

        ImGui::Dummy(ImVec2(node->w_node, 10.0f));
    }

    m_active_node = nullptr;
}

void NodeDrawer::draw_node_header(data::NodeInstance* node)
{
    if (nullptr == m_renaming_node)
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + node->w_node_name_off);
        ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_header_text);
        ImGui::TextUnformatted(node->get_name().c_str(), node->get_name().c_str() + node->get_name().size());
        ImGui::PopStyleColor();

        if (node->pinned())
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeHeaderIcon));
            ImGui::TextUnformatted(" " ICON_LC_PIN);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) m_active_tt = "Node is pinned to the canvas.";
        }
        if (node->get_mode() == NodeMode::Bypass)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeHeaderIcon));
            ImGui::TextUnformatted(" " ICON_LC_SKIP_FORWARD);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) m_active_tt = "Node is bypassed. Inputs pass through to outputs.";
        }
        else if (node->get_mode() != NodeMode::Always)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeHeaderIcon));
            ImGui::TextUnformatted(" " ICON_LC_VOLUME_X);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) m_active_tt = "Node is muted. It will not be processed.";
        }
    }
    else if (m_renaming_node == node)
    {
        if (m_need_renaming_focus)
        {
            ImGui::SetKeyboardFocusHere();
            m_need_renaming_focus = false;
        }
        ImGui::SetCursorPosX(m_curr_node_left_offset + ImGui::GetStyle().ItemSpacing.x * 2);
        ImGui::SetNextItemWidth(node->w_node - ImGui::GetStyle().ItemSpacing.x * 4);
        if (ImGuiEx::InputText("##noderename", &m_renaming_buff, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue) ||
            ImGui::IsItemDeactivatedAfterEdit())
        {
            m_renaming_node->set_name(tools::str::trim_this(m_renaming_buff));
            m_renaming_node = nullptr;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_renaming_node = nullptr;
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        m_renaming_node = node;
        m_renaming_buff = node->get_name();
        m_need_renaming_focus = true;
    }
}

void NodeDrawer::draw_node_header_bg(data::NodeInstance* node)
{
    auto draw_list = ed::GetNodeBackgroundDrawList(cast::to_ptr(node));
    if (draw_list != nullptr)
    {
        auto& style = ed::GetStyle();

        const ImU32 bg_color = (node->m_bg_color_custom ? node->m_header_color : m_clr_node_header_bg);
        const float header_h = ImGui::GetTextLineHeight() * 1.5f;

        draw_list->AddRectFilled(
            ImVec2(m_curr_node_left_offset - style.NodePadding.x + style.NodeBorderWidth, m_curr_node_top_offset - style.NodePadding.y + style.NodeBorderWidth),
            ImVec2(m_curr_node_left_offset + style.NodePadding.x + node->w_node - style.NodeBorderWidth, m_curr_node_top_offset + header_h),
            bg_color,
            style.NodeRounding,
            node->expanded () ? ImDrawFlags_RoundCornersTop : ImDrawFlags_RoundCornersAll 
        );
    }
}

void NodeDrawer::draw_input_pin(data::PinInstance* pin)
{
    if (m_active_node->expanded())
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw);
        draw_pin(pin, false);
        ImGui::SameLine(0.0f, w::pin_r_offset);
        ImGui::TextUnformatted(pin->get_object()->name.c_str(), pin->get_object()->name.c_str() + pin->get_object()->name.size());
    }
    else if (pin->m_state->links > 0)
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset);
        draw_pin(pin, true);
    }
}

void NodeDrawer::draw_output_pin(data::PinInstance* pin)
{
    if (m_active_node->expanded())
    {
        ImGui::TextUnformatted(pin->get_object()->name.c_str(), pin->get_object()->name.c_str() + pin->get_object()->name.size());
        ImGui::SameLine(0, w::pin_r_offset);
        draw_pin(pin, false);
    }
    else if (pin->m_state->links > 0)
    {
        draw_pin(pin, true);
    }
}

void NodeDrawer::draw_pin(data::PinInstance* pin, bool force_default_shape)
{
    ImGui::PushID(&pin->m_state->pin_color);
#ifdef _DEBUG
    test_id(m_ids_imgui, &pin->m_state->pin_color);
    test_id(m_ids_nodelib, pin);
#endif
    ed::BeginPin(cast::to_ptr(pin), pin->get_object()->input ? ed::PinKind::Input : ed::PinKind::Output);
    draw_pin_shape(pin, force_default_shape);
    ed::EndPin();
    ImGui::PopID();
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
        if (m_active_node->expanded())
        {
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw);
            draw_pin(pin, false);
            ImGui::SameLine(0, w::pin_r_offset);
            draw_widget(pin, widget_name_width, widget_value_width);
        }
        else if (pin->m_state->links > 0)
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(m_curr_node_left_offset);
            draw_pin(pin, true);
        }
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
#ifdef _DEBUG
    test_id(m_ids_imgui, val);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->m_state->links > 0;
    if (linked) ImGui::BeginDisabled();
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
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state();

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
#ifdef _DEBUG
    test_id(m_ids_imgui, val);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->m_state->links > 0;
    if (linked) ImGui::BeginDisabled();
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
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state();

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
#ifdef _DEBUG
    test_id(m_ids_imgui, val);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->m_state->links > 0;
    if (linked) ImGui::BeginDisabled();
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
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state();

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;
    bool* val = data::Pin::get_bool(state->value);

    ImGui::PushID(val);
#ifdef _DEBUG
    test_id(m_ids_imgui, val);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 0.0f);

    const auto linked = pin->m_state->links > 0;
    if (linked) ImGui::BeginDisabled();
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
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state();

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    auto& placeholder = (obj->placeholder.empty() ? obj->name : obj->placeholder);
    std::string* val = data::Pin::get_string(pin->m_state->value);

    ImGui::PushID(val);
#ifdef _DEBUG
    test_id(m_ids_imgui, val);
#endif

    ImGui::PushItemWidth(widget_name_width + widget_value_width);
    const auto linked = pin->m_state->links > 0;
    if (linked) ImGui::BeginDisabled();
    ImGuiEx::InputTextWithHint("##value", placeholder.c_str(), val);
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state();
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
#ifdef _DEBUG
    test_id(m_ids_imgui, val);
#endif

    ImGui::PushTextWrapPos(0.0f);
    ImGui::SetNextItemWidth(widget_name_width + widget_value_width);
    const auto linked = pin->m_state->links > 0;
    if (linked) ImGui::BeginDisabled();
    ImGuiExNe::InputTextWithHint(*m_deffered_action.get(), m_active_node, pin, "##value", placeholder.c_str(), val);
    if (linked) ImGui::EndDisabled();
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
#ifdef _DEBUG
    test_id(m_ids_imgui, &pin->m_state->combo_index);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->m_state->links > 0;
    if (linked) ImGui::BeginDisabled();
    ImGuiExNe::Combo(*m_deffered_action.get(), m_active_node, pin, "##value", &pin->m_state->combo_index, obj->combo_items_ptr.data(), cast::to_i32(obj->combo_items_ptr.size()));
    if (linked) ImGui::EndDisabled();
    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->m_state;

    ImGui::PushID(state->colors);
#ifdef _DEBUG
    test_id(m_ids_imgui, state->colors);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &obj->name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state();
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    auto colors = state->colors;
    const auto linked = pin->m_state->links > 0;
    if (linked) ImGui::BeginDisabled();
    if (ImGui::ColorEdit4("##value", colors,
        ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop |
        ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoTooltip | (obj->alpha_enabled ? 0 : ImGuiColorEditFlags_NoAlpha)))
    {
        state->value = data::Pin::colors_to_int(state->colors);
    }
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state();

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
#ifdef _DEBUG
            test_id(m_ids_imgui, &state->combo_index);
#endif

            ImGui::SetNextItemWidth(widget_name_width);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::InputText("##CAGControlName", "control", ImGuiInputTextFlags_ReadOnly);
            handle_widget_active_state();
            ImGui::PopStyleColor();

            ImGui::SameLine(0.0f, 0.0f);
            ImGui::SetNextItemWidth(widget_value_width);
            const auto linked = pin->m_state->links > 0;
            if (linked) ImGui::BeginDisabled();
            ImGuiExNe::Combo(*m_deffered_action.get(), m_active_node, pin, "##CAGComboValue", &state->control_after_generate_value, data::ControlAfterGenerateStr, sizeof(data::ControlAfterGenerateStr) / sizeof(data::ControlAfterGenerateStr[0]));
            if (linked) ImGui::EndDisabled();
            ImGui::PopID();
        }
    }
}

void NodeDrawer::draw_widget_separator(float offset, bool& opened, bool& temp_open)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + offset);

    ImGui::PushFont(g::pMain->get_small_font());

    ImGui::PushStyleColor(ImGuiCol_Text, 0);
    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
    const bool pressed = ImGui::Button(ICON_LC_CHEVRON_UP);
    const auto btn_pos = ImGui::GetItemRectMin();
    ImGui::PopStyleColor(4);

    temp_open = false;
    ImU32 color = ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);
    if (pressed)
    {
        opened = !opened;
        ed::SelectNode(cast::to_ptr(m_active_node));
    }
    else if (ImGui::IsItemHovered())
        temp_open = true;
    else
        color = g::pOptsManager->color_load(AppColor::ActivityBarInactive);

    ImGui::GetWindowDrawList()->AddText(btn_pos, color, opened || temp_open ? ICON_LC_CHEVRON_UP : ICON_LC_CHEVRON_DOWN);
    ImGui::PopFont();
}

void NodeDrawer::draw_links(const std::vector<std::unique_ptr<data::Link>>& links)
{
    //static int* test = new int(55);
    //static bool test_b = true, test_pos = true;
    //if (test_b)
    //{
    //    ed::BeginNode(ed::NodeId(test));
    //    ed::Group(ImVec2(200, 200));
    //    ed::EndNode();

    //    if (test_pos)
    //        ed::SetNodePosition(ed::NodeId(test), ImVec2(50, 50));

    //    auto pos1 = ed::GetNodePosition(ed::NodeId(test));
    //    auto pos2 = ed::GetNodeSize(ed::NodeId(test));
    //    (void)pos1;
    //    (void)pos2;

    //    test_pos = false;
    //}

    //static int* test2 = new int(55);
    //static bool test_b2 = true, test_pos2 = true;
    //if (test_b2)
    //{
    //    if (ed::BeginGroupHint(ed::NodeId(test2)))
    //    {
    //    }
    //    ed::EndGroupHint();
    //}

    for (auto& link : links)
    {
#ifdef _DEBUG
        test_id(m_ids_nodelib, link.get());
#endif
        auto in = link->get_input();
        auto out = link->get_output();
        ed::Link(cast::to_ptr(link.get()), cast::to_ptr(in), cast::to_ptr(out), in->m_state->pin_color);
    }
}

bool NodeDrawer::query_links()
{
    bool linked = false;
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
                    linked = true;
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

    return linked;

    //static data::PinInstance* test = nullptr;
    //if (test != m_querying_pin)
    //{
    //    test = m_querying_pin;
    //    if (test != nullptr)
    //        ::OutputDebugStringA(fmt::format("Drag pin {}", test->get_object()->name).c_str());
    //    else
    //        ::OutputDebugStringA("Drag pin is 0");
    //}
}

void NodeDrawer::draw_pin_shape(data::PinInstance* pin, bool force_default)
{
    const bool querying = (m_querying_pin != nullptr);
    const bool compatible = (m_querying_pin == pin || (querying && data::Link::can_link(pin, m_querying_pin)));
    const bool connected = (m_querying_pin == pin || pin->m_state->links > 0);
    const auto type = (force_default ? data::PinShape::Circle : data::Pin::get_shape(pin->m_state->type));
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

bool NodeDrawer::handle_node_resize()
{
    if (m_resizing_grip_positions.empty())
        return false;

    m_now_resizing = -1;
    const ImVec2 old_pos = ImGui::GetCursorScreenPos();

    for (int32_t idx = 0; idx < m_resizing_grip_positions.size(); idx++)
    {
        ImGui::SetCursorScreenPos(m_resizing_grip_positions[idx].Min);

        ImGui::PushID(idx);
        ImGui::InvisibleButton("##grip", ImVec2(m_grip_area_sz, m_grip_area_sz));
        ImGui::PopID();

        const bool active = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped);
        if (m_resizing_w[idx] >= 0 && (active || hovered))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

            if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                m_now_resizing = idx;
                ImVec2 p1 = ed::ScreenToCanvas(ImVec2(0, 0));
                ImVec2 p2 = ed::ScreenToCanvas(ImGui::GetIO().MouseDelta);
                ImVec2 canvas_delta = ImVec2(p2.x - p1.x, p2.y - p1.y);
                m_resizing_w[m_now_resizing] += canvas_delta.x;
                m_resizing_w[m_now_resizing] = std::max(0.0f, m_resizing_w[m_now_resizing]);
            }
        }
    }

    ImGui::SetCursorScreenPos(old_pos);
    return m_now_resizing != -1;
}

bool NodeDrawer::handle_context_menu()
{
    if (ed::ShowBackgroundContextMenu())
    {        
        m_menuid_activate = m_menuid_canvas;
        return true;
    }

    if (!ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        return false;

    const auto pin = ed::GetHoveredPin();
    if (pin)
    {
        m_menuid_activate = m_menuid_pin;
        m_menu_object = cast::ptrs<void>((std::uintptr_t)pin);
        //m_click_pos = ed::ScreenToCanvas(ImGui::GetMousePos());
        return true;
    }

    const auto node = ed::GetHoveredNode();
    if (node)
    {
        m_menuid_activate = m_menuid_node;
        m_menu_object = cast::ptrs<void>((std::uintptr_t)node);
        //m_click_pos = ed::ScreenToCanvas(ImGui::GetMousePos());
        return true;
    }

    const auto link = ed::GetHoveredLink();
    if (link)
    {
        m_menuid_activate = m_menuid_link;
        m_menu_object = cast::ptrs<void>((std::uintptr_t)link);
        //m_click_pos = ed::ScreenToCanvas(ImGui::GetMousePos());
        return true;
    }

    //m_menuid_activate = m_menuid_canvas;
    //m_click_pos = ed::ScreenToCanvas(ImGui::GetMousePos());
    return true;
}

bool NodeDrawer::handle_canvas_menu()
{
    bool ret = false;
    if (ImGui::BeginPopup(m_menuid_canvas))
    {
        if (ImGui::MenuItem("Back menu"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
        ret = true;
    }
    return ret;
}

bool NodeDrawer::handle_node_menu()
{
    // todo: group click
    bool ret = false;
    static float color[4] = { -1.0f };
    auto node = cast::ptrs<data::NodeInstance>(m_menu_object);

    if (ImGui::BeginPopup(m_menuid_node))
    {
        bool close = false;
        if (ImGui::MenuItem("Rename"))
        {
            m_renaming_node = node;
            m_renaming_buff = node->get_name();
            m_need_renaming_focus = true;
            close = true;
        }
        if (ImGui::MenuItem(node->pinned() ? "Unpin" : "Pin"))
        {
            node->toggle_pinned();
            ed::SetNodeDraggable(cast::to_ptr(node), !node->pinned());
            close = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Always", nullptr, node->get_mode() == NodeMode::Always))
        {
            node->set_mode(NodeMode::Always);
            close = true;
        }
        if (ImGui::MenuItem("Bypass", nullptr, node->get_mode() == NodeMode::Bypass))
        {
            node->set_mode(NodeMode::Bypass);
            close = true;
        }
        if (ImGui::MenuItem("Mute", nullptr, node->get_mode() == NodeMode::Mute || node->get_mode() == NodeMode::OnTrigger || node->get_mode() == NodeMode::OnEvent))
        {
            node->set_mode(NodeMode::Mute);
            close = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Remove")) // TODO
        {
            m_callbacks.delete_node(node);
            close = true;
        }
        if (ImGui::MenuItem("Clone")) // TODO
        {
            m_callbacks.copy_node(node);
            close = true;
        }
        if (ImGui::MenuItem("Copy")) // TODO : paste
        {
            m_copied_node = node;
            close = true;
        }
        if (ImGui::MenuItem("Paste", nullptr, false, m_copied_node != nullptr)) // TODO : paste
        {
            m_callbacks.copy_node(m_copied_node);
            close = true;
        }
        ImGui::Separator();
        //if (ImGui::MenuItem(node->expanded() ? "Collapse" : "Expand")) // TODO: FIX
        //{
        //    node->toggle_expanded();
        //    close = true;
        //}
        //if (ImGui::MenuItem("Fit to size"))  // TODO
        //{
        //    close = true;
        //}
        ////ImGui::Separator();
        //if (ImGui::MenuItem("Convert to subgraph")) // TODO
        //{
        //    close = true;
        //}
        //ImGui::Separator();
        if (ImGui::BeginMenu("Color"))
        {
            if (ImGui::MenuItem("Reset to default", nullptr, nullptr, node->m_bg_color_custom))
            {
                node->m_bg_color_custom = false;
                close = true;
            }
            ImGui::Separator();

            const std::pair<ImU32, const char*> colors[] =
            {
                { IM_COL32(165, 42, 42, 255), "Red" },
                { IM_COL32(42, 165, 42, 255), "Green" },
                { IM_COL32(42, 42, 165, 255), "Blue" },
                { IM_COL32(90, 110, 130, 255), "Pale blue" },
                { IM_COL32(42, 165, 165, 255), "Cyan" },
                { IM_COL32(105, 42, 165, 255), "Purple" },
                { IM_COL32(165, 165, 42, 255), "Yellow" },
                { IM_COL32(110, 70, 40, 255), "Brown" },
                { IM_COL32(0, 0, 0, 255), "Black" },
            };

            for (auto& [clr, name] : colors)
            {
                ImGui::PushID(clr);
                bool sel = ImGui::ColorButton("", ImGui::ColorConvertU32ToFloat4(clr), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder);
                ImGui::SameLine();
                if (ImGui::MenuItem(name) || sel)
                {
                    node->m_bg_color_custom = true;
                    node->m_bg_color = ImGui::ColorConvertU32ToFloat4(clr);
                    node->m_header_color = ImGuiEx::decrease_brightness(ImGui::ColorConvertFloat4ToU32(node->m_bg_color));
                    close = true;
                }
                ImGui::PopID();
            }

            ImGui::Spacing();
            if (color[0] < 0.0f)
            {
                if (node->m_bg_color_custom)
                {
                    color[0] = node->m_bg_color.x; color[1] = node->m_bg_color.y; color[2] = node->m_bg_color.z; color[3] = node->m_bg_color.w;
                }
                else
                {
                    auto vec = ed::GetStyle().Colors[ed::StyleColor_NodeBg];
                    color[0] = vec.x; color[1] = vec.y; color[2] = vec.z; color[3] = vec.w;
                }
            }
            if (ImGui::ColorPicker4("###Background color", color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_DefaultOptions_))
            {
                node->m_bg_color.x = color[0]; node->m_bg_color.y = color[1]; node->m_bg_color.z = color[2]; node->m_bg_color.w = color[3];
                node->m_header_color = ImGuiEx::decrease_brightness(ImGui::ColorConvertFloat4ToU32(node->m_bg_color));
                node->m_bg_color_custom = true;
            }

            ImGui::EndMenu();
        }

        if (close)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        ret = true;
    }
    else
        color[0] = -1.0f;
    return ret;
}

bool NodeDrawer::handle_pin_menu()
{
    bool ret = false;
    if (ImGui::BeginPopup(m_menuid_canvas))
    {
        if (ImGui::MenuItem("Back menu"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
        ret = true;
    }
    return ret;
}

bool NodeDrawer::handle_link_menu()
{
    bool ret = false;
    if (ImGui::BeginPopup(m_menuid_canvas))
    {
        if (ImGui::MenuItem("Back menu"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
        ret = true;
    }
    return ret;
}

} // namespace ui
