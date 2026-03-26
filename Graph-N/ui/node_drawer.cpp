
#include "ui/node_drawer.h"
#include "ui/helpers/imgui_ex_ne.h"
#include "ui/graph-n_flow.h"
#include "main.h"
#include "core/managers/settings_manager.h"
#include "core/managers/history_manager.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "core/workflow/link.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "imgui_node_editor.h"

namespace ui
{

namespace ed = ax::NodeEditor;

NodeDrawer::NodeDrawer(data::Workflow* wf)
    : m_wf(wf)
    , m_clr_node_header_text(g::pOptsManager->color_load(AppColor::NodeHeaderText))
    , m_clr_node_pin_text(g::pOptsManager->color_load(AppColor::NodePinText))    
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

void NodeDrawer::init(blob::Blob&& state)
{
    LogF;

    m_deffered_action = std::make_unique<ImGuiExNe::DeferredAction>();

    m_state = std::move(state);
    const auto load_state = [this](ImVec2& view_scroll, float& view_zoom, ImRect& visible_rect)
    {
        if (!m_state->empty())
        {
            blob::Reader reader(m_state);
            view_scroll = reader.pop_vec2();
            view_zoom = reader.pop_float();
            visible_rect = reader.pop_rect();
        }
        else
            view_zoom = 1.0f;
    };

    const auto save_state = [this](const ImVec2& view_scroll, float view_zoom, const ImRect& visible_rect)
    {
        blob::Writer writer;
        writer
            .push(view_scroll)
            .push(view_zoom)
            .push(visible_rect);
        m_state = writer.release();
    };

    ed::Config config;
    config.LoadState = load_state; 
    config.SaveState = save_state; 
    config.EnableSmoothZoom = true;
    m_context = ed::CreateEditor(&config);
}

void NodeDrawer::free()
{
    LogF;

    m_deffered_action.reset();
    m_state->clear();

    if (m_context != nullptr)
        ed::DestroyEditor(m_context);
    m_context = nullptr;
}

void NodeDrawer::locate_node(data::NodeInstance* node)
{
    auto prev_context = ed::GetCurrentEditor();
    if (nullptr == prev_context)
        ed::SetCurrentEditor(m_context);

    ed::ClearSelection();
    ed::SelectNode(node->ed());
    ed::NavigateToSelection(false, 0.5f);

    if (nullptr == prev_context)
        ed::SetCurrentEditor(nullptr);
}

data::PinInstance* NodeDrawer::get_ml_widget_active() const
{
    if (nullptr == m_deffered_action->active_pin)
        return nullptr;
    return m_deffered_action->active_pin;
}

void NodeDrawer::pre_draw_all(const char* id, size_t /*nodes_count*/)
{
    //m_resizing_grip_positions.clear(); // TODO:FIX resizing
    //m_resizing_grip_positions.resize(nodes_count);

    m_active_tt = nullptr;

    //if (m_resizing_w.size() != nodes_count) // TODO:FIX resizing
    //{
    //    m_now_resizing = -1;
    //    m_resizing_w.clear();
    //    m_resizing_w.resize(nodes_count);
    //}

    ed::EditConfig(m_context).AlignPointToGrid = g::pOptsManager->wf_snap_nodes_load();
    ed::EditConfig(m_context).DrawGrid = g::pOptsManager->wf_show_grid_load();
    ed::SetCurrentEditor(m_context);
    ed::EnableShortcuts(false);

    handle_deleted();

    //if (!m_view_set)
    //{
    //    m_view_set = true;
    //    ed::SetCurrentView(m_view_rect);
    //}
    //if (!m_zoom_set)
    //{
    //    m_zoom_set = true;
    //    ed::SetCurrentZoom(m_zoom);
    //}

    m_defclr_node_bg = g::pOptsManager->color_load(AppColor::NodeBodyBg);
    m_defclr_node_header_bg = ImGuiEx::decrease_brightness(m_defclr_node_bg);

    ed::Begin(id, ImVec2(0, 0));
}

void NodeDrawer::post_draw_all(
    const std::function<bool()>& init_menu_handler, const std::function<bool()>& show_menu_handler, const std::function<bool()>& move_handler)
{
    ed::Suspend();

    bool canvas_busy = init_menu_handler();
    //if (!canvas_busy)
    //    canvas_busy = handle_node_resize();   // TODO:FIX resizing

    ed::Resume();

    if (!canvas_busy)
        canvas_busy = move_handler();

    ed::End();

    canvas_busy = show_menu_handler();

    if (!canvas_busy && m_deffered_action->combo.is_active)
    {
        ImGuiExNe::RenderDeferredCombo(*m_deffered_action.get());
    }
    else if (!canvas_busy && m_deffered_action->ml_input.is_active)
    {
        g::pFlow->txt_panel_create(
            m_wf->get_id(),
            m_deffered_action->active_node,
            m_deffered_action->active_pin,
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
    if (node->m_group)
    {
        ed::PushStyleColor(ed::StyleColor_GroupBg, ImVec4(0, 0, 0, 0));
        ed::PushStyleColor(ed::StyleColor_GroupBorder, ImVec4(0, 0, 0, 0));
        ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0, 0, 0, 0));
    }
    else
    {
        if (node->get_object()->is_dummy)
        {
            const auto clr = ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::DummyNode));
            ed::PushStyleColor(ed::StyleColor_NodeBorder, clr);
            ed::PushStyleColor(ed::StyleColor_HovNodeBorder, clr);
            ed::PushStyleColor(ed::StyleColor_SelNodeBorder, clr);
            ed::PushStyleColor(ed::StyleColor_NodeSelRect, clr);
            ed::PushStyleColor(ed::StyleColor_NodeSelRectBorder, clr);
        }
        ed::PushStyleColor(ed::StyleColor_NodeBg, node->m_clr_node);
    }

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

    if (!node->m_pos_set)
    {
        if (node->ed())
            ed::SetNodePosition(node->ed(), node->m_rc.Min);
        else
            node->ed() = ed::SetNodePosition(cast::to_ptr(node), node->m_rc.Min);
        node->m_pos_set = true;
    }
    if (!node->m_size_set && node->m_group)
    {
        if (node->ed())
            ed::SetGroupSize(node->ed(), node->m_rc.Max);
        else
            node->ed() = ed::SetGroupSize(cast::to_ptr(node), node->m_rc.Max);
        node->m_size_set = true;
    }

    if (node->ed())
        ed::BeginNode(node->ed());
    else
        node->ed() = ed::BeginNode(cast::to_ptr(node));

    if (!node->m_pinned_set)
    {
        ed::SetNodeDraggable(node->ed(), !node->pinned());
        node->m_pinned_set = true;
    }

    const bool deferred_node_active = m_deffered_action->active_node == node;
    if (deferred_node_active)
    {
        if (ed::HasSelectionChanged())
            m_deffered_action->active_node = nullptr;
    }
}

void NodeDrawer::post_draw_node(data::NodeInstance* node)
{
    ed::EndNode();
    
    node->m_rc.Min = ed::GetNodePosition(node->ed());
    if (!node->m_ignore_size_on_frame_end)
        node->m_rc.Max = ed::GetNodeSize(node->ed());
    node->m_ignore_size_on_frame_end = false;

    if (node->m_group)
        ed::PopStyleColor(3);
    else if (node->get_object()->is_dummy)
        ed::PopStyleColor(6);
    else
        ed::PopStyleColor();

    if (node->get_mode() != NodeMode::Always)
    {
        ed::PopStyleColor();
        ImGui::PopStyleVar();
    }

    m_active_node = nullptr;
}

void NodeDrawer::draw_node(data::NodeInstance* node, float node_w, size_t /*node_idx*/)
{
    m_active_node = node;

#ifdef _DEBUG
    m_ids_imgui.clear();
    m_ids_nodelib.clear();
    m_ids_imgui.insert(m_active_node);
    m_ids_nodelib.insert(m_active_node);
#endif

    ImVec2 node_size/*, node_pos*/;

    //if (m_now_resizing == node_idx) // TODO:FIX resizing
    //{
    //    node_w = m_resizing_w[node_idx];
    //}
    //else
    {
        node_size = ed::GetNodeSize(m_active_node->ed());
        if (node_size.x > 0 && node_size.y > 0)
        {
            //node_pos = ed::GetNodePosition(m_active_node->ed());
            node_w = node_size.x - ed::GetStyle().NodePadding.x - ed::GetStyle().NodePadding.z;
        }
    }

    if (!m_active_node->m_size_set && m_active_node->m_rc.Max.x > 0)
    {
        node_w = m_active_node->m_rc.Max.x;
        m_active_node->m_size_set = true;
    }
    node_w = m_active_node->recalc_node_width(node_w, false, false);

    //if (node_size.x > 0 && node_size.y > 0) // TODO:FIX resizing
    //{
    //    m_resizing_w[node_idx] = (m_active_node->pinned() || !m_active_node->expanded() ? -1 : node_w);
    //    if (m_resizing_w[node_idx] > 0)
    //    {
    //        m_resizing_grip_positions[node_idx].Min =
    //            ed::CanvasToScreen(ImVec2(node_pos.x + node_w + ed::GetStyle().NodePadding.x + ed::GetStyle().NodeBorderWidth - m_grip_area_sz,
    //                node_pos.y + node_size.y + ed::GetStyle().NodePadding.y + ed::GetStyle().NodeBorderWidth - m_grip_area_sz));
    //        m_resizing_grip_positions[node_idx].Max =
    //            ed::CanvasToScreen(ImVec2(node_pos.x + node_w + ed::GetStyle().NodePadding.x + ed::GetStyle().NodeBorderWidth + m_grip_area_sz,
    //                node_pos.y + node_size.y + ed::GetStyle().NodePadding.y + ed::GetStyle().NodeBorderWidth + m_grip_area_sz));
    //    }
    //    //m_resizing_grip_positions[node_idx] = { br.x - m_grip_area_sz, br.y - m_grip_area_sz, br.x + m_grip_area_sz, br.y };
    //}

    m_curr_node_left_offset = ImGui::GetCursorPosX();
    m_curr_node_top_offset = ImGui::GetCursorPosY();

    // --- HEADER ---

    //ImGui::Dummy(ImVec2(m_active_node->w_node, w::v_spacing));
    draw_node_header();
    if (!m_active_node->get_expanded())
    {
        m_active_node = nullptr;
        return;
    }

    ImGui::Dummy(ImVec2(m_active_node->w_node, ImGui::GetTextLineHeight() * 0.5f));
    const auto pin_y = ImGui::GetCursorPosY();

    // --- PINS - INPUT ---

    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_pin_text);

    const auto& input = m_active_node->get_inputs();
    for (auto pin : input)
    {
        if (!pin->get_object()->is_widget())
        {
            ImGui::PushID(pin);
#ifdef _DEBUG
            test_id(m_ids_imgui, pin);
#endif
            draw_input_pin(pin);
            ImGui::PopID();
        }
    }

    const auto last_in_off_y = ImGui::GetCursorPosY();

    // --- SPACER ---

    if (!m_active_node->get_outputs().empty())
    {
        ImGui::SetCursorPosY(pin_y);
        if (m_active_node->w_center > 0.0f)
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::ico_max + w::pin_r_offset + m_active_node->w_pin_in + m_active_node->w_center);
    }

    // --- PINS - OUTPUT ---

    const auto out_pins_pos = ImGui::GetCursorPosX();

    const auto& output = m_active_node->get_outputs();
    for (auto pin : output)
    {
        ImGui::PushID(pin);
#ifdef _DEBUG
        test_id(m_ids_imgui, pin);
#endif
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::ico_max + w::pin_r_offset + m_active_node->w_pin_in + m_active_node->w_center +
            m_active_node->w_pin_out - pin->state().w_name);

        draw_output_pin(pin);
        ImGui::PopID();
    }

    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(m_active_node->w_node, w::v_spacing * 2));

    // TODO:FIX resizing
   // m_resizing_grip_positions[node_idx].Min.y -= ImGui::GetCursorPos().y - m_curr_node_top_offset + ImGui::GetTextLineHeight();

    // --- WIDGETS ---

    if (m_active_node->m_has_widgets)
    {
        ImGui::SetCursorPosY(std::max(last_in_off_y, ImGui::GetCursorPosY()));

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
                draw_input_widget(pin, m_active_node->w_widget_name, m_active_node->w_widget_value);
                ImGui::PopID();
            }
        }
        if (advanced > 0)
        {
            bool temp_open = false;
            draw_widget_separator(w::pin_l_offset_rw + w::ico_max + w::pin_r_offset, m_active_node->m_show_hidden, temp_open);
            if (m_active_node->m_show_hidden || temp_open)
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
                        draw_input_widget(pin, m_active_node->w_widget_name, m_active_node->w_widget_value);
                        ImGui::PopID();
                    }
                }
            }
        }
        else
            m_active_node->m_show_hidden = false;

        ImGui::Dummy(ImVec2(m_active_node->w_node, 10.0f));
    }

    m_active_node = nullptr;
}

void NodeDrawer::draw_group(data::NodeInstance* node, size_t /*node_idx*/)
{
    m_active_node = node;

    m_curr_node_left_offset = ImGui::GetCursorScreenPos().x;
    m_curr_node_top_offset = ImGui::GetCursorScreenPos().y;

    auto& style = ed::GetStyle();

    const float rc_x = m_curr_node_left_offset - style.NodePadding.x + style.GroupBorderWidth;
    const ImVec2 text_lt(m_curr_node_left_offset + ImGui::GetStyle().ItemSpacing.x, m_curr_node_top_offset);
    const float width = m_curr_node_left_offset + m_active_node->m_rc.Max.x - style.NodePadding.x - style.GroupBorderWidth;
    const float capt_height = m_curr_node_top_offset + ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;

    auto draw_list = ImGui::GetWindowDrawList();
    if (draw_list != nullptr)
    {
        draw_list->AddRectFilled(
            ImVec2(rc_x, m_curr_node_top_offset - style.NodePadding.y + style.GroupBorderWidth),
            ImVec2(width, capt_height),
            m_active_node->m_clr_head,
            style.NodeRounding,
            ImDrawFlags_RoundCornersTop
        );

        draw_list->AddRectFilled(
            ImVec2(rc_x, m_curr_node_top_offset + ImGui::GetTextLineHeight()),
            ImVec2(width, m_curr_node_top_offset + m_active_node->m_rc.Max.y - style.NodePadding.y),
            m_active_node->m_clr_group,
            style.NodeRounding,
            ImDrawFlags_RoundCornersBottom
        );

        if (m_active_node != m_renaming_node)
        {
            ImGui::SetCursorPosX(text_lt.x);
            ImGui::PushStyleColor(ImGuiCol_Text, m_active_node->m_clr_group_capt);
            ImGui::TextUnformatted(m_active_node->get_name().c_str(), m_active_node->get_name().c_str() + m_active_node->get_name().size());

            //width = m_active_node->m_rc.Max.x - ImGui::GetStyle().ItemSpacing.x * 4; // TODO:FIX name too long: use draw_list->AddText
            //const char* pend = nullptr;
            //const char* pbeg = ImGuiEx::cut_text_with_ellipsis(m_active_node->get_name(), width, pend);
            //ImGui::TextUnformatted(pbeg, pend);

            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                m_renaming_node = m_active_node;
                m_renaming_buff = m_active_node->get_name();
                m_need_renaming_focus = true;
            }
        }
    }

    if (m_renaming_node == m_active_node)
    {
        if (m_need_renaming_focus)
        {
            ImGui::SetKeyboardFocusHere();
            m_need_renaming_focus = false;
        }
        ImGui::SetCursorPosX(m_curr_node_left_offset + ImGui::GetStyle().ItemSpacing.x * 2);
        ImGui::SetNextItemWidth(m_active_node->m_rc.Max.x - style.NodePadding.x * 2 - style.GroupBorderWidth * 2 - ImGui::GetStyle().ItemSpacing.x * 4);
        if (ImGuiEx::InputText("##grouprename", &m_renaming_buff, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue) ||
            ImGui::IsItemDeactivatedAfterEdit())
        {
            m_wf->action()->set_node_name(m_renaming_node, m_renaming_buff);
            m_renaming_buff.clear();
            m_renaming_node = nullptr;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_renaming_node = nullptr;
        }
    }

    ed::Group(m_active_node->m_rc.Max);
}

void NodeDrawer::draw_node_header()
{
    auto draw_list = ImGui::GetWindowDrawList();
    if (draw_list != nullptr)
    {
        auto& style = ed::GetStyle();

        //const ImU32 bg_color = (m_active_node->m_clr_custom ? m_active_node->m_clr_head : m_defclr_node_header_bg);
        const float header_h = ImGui::GetTextLineHeight() * 1.5f - (m_active_node->get_expanded() ? 0 : style.NodeBorderWidth);

        draw_list->AddRectFilled(
            ImVec2(m_curr_node_left_offset - style.NodePadding.x + style.NodeBorderWidth, m_curr_node_top_offset - style.NodePadding.y + style.NodeBorderWidth),
            ImVec2(m_curr_node_left_offset + style.NodePadding.x + m_active_node->w_node - style.NodeBorderWidth, m_curr_node_top_offset + header_h),
            m_active_node->m_clr_head,
            style.NodeRounding,
            (m_active_node->get_expanded() ? ImDrawFlags_RoundCornersTop : ImDrawFlags_RoundCornersAll)
        );
    }

    const float collapsed_pin_y = ImGui::GetCursorPosY();
    if (!m_active_node->get_expanded() && !m_active_node->get_inputs().empty())
    {
        draw_collapsed_pins(true);
        ImGui::SameLine();
    }

    if (m_active_node != m_renaming_node)
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + m_active_node->w_node_name_off);
        ImGui::PushStyleColor(ImGuiCol_Text, m_active_node->get_object()->is_dummy ? g::pOptsManager->color_load(AppColor::DummyNode) : m_clr_node_header_text);
        ImGui::TextUnformatted(m_active_node->get_name().c_str(), m_active_node->get_name().c_str() + m_active_node->get_name().size());
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            m_renaming_node = m_active_node;
            m_renaming_buff = m_active_node->get_name();
            m_need_renaming_focus = true;
        }

        if (m_active_node->pinned())
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeHeaderIcon));
            ImGui::TextUnformatted(" " ICON_LC_PIN);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) m_active_tt = "Node is pinned to the canvas.";
        }
        if (m_active_node->get_mode() == NodeMode::Bypass)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeHeaderIcon));
            ImGui::TextUnformatted(" " ICON_LC_SKIP_FORWARD);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) m_active_tt = "Node is bypassed. Inputs pass through to outputs.";
        }
        else if (m_active_node->get_mode() != NodeMode::Always)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeHeaderIcon));
            ImGui::TextUnformatted(" " ICON_LC_VOLUME_X);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) m_active_tt = "Node is muted. It will not be processed.";
        }
    }
    else if (m_renaming_node == m_active_node)
    {
        if (m_need_renaming_focus)
        {
            ImGui::SetKeyboardFocusHere();
            m_need_renaming_focus = false;
        }
        if (m_active_node->get_expanded())
        {
            ImGui::SetCursorPosX(m_curr_node_left_offset + ImGui::GetStyle().ItemSpacing.x * 2);
            ImGui::SetNextItemWidth(m_active_node->w_node - ImGui::GetStyle().ItemSpacing.x * 4);
        }
        else
        {
            ImGui::SetCursorPosX(m_curr_node_left_offset + (m_renaming_node->get_inputs().empty() ? 0 : (w::pin_l_offset_rw + w::ico_circle + w::pin_r_offset + ImGui::GetStyle().ItemSpacing.x)));
            ImGui::SetNextItemWidth(m_active_node->w_node - (m_renaming_node->get_outputs().empty() ? 0 : ((w::pin_l_offset_rw + w::ico_circle + w::pin_r_offset + ImGui::GetStyle().ItemSpacing.x) * 2)));
        }
        if (ImGuiEx::InputText("##noderename", &m_renaming_buff, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue) ||
            ImGui::IsItemDeactivatedAfterEdit())
        {
            m_wf->action()->set_node_name(m_renaming_node, m_renaming_buff);
            m_renaming_buff.clear();
            m_renaming_node = nullptr;
            m_renaming_pin = nullptr;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_renaming_buff.clear();
            m_renaming_node = nullptr;
            m_renaming_pin = nullptr;
        }
    }

    if (!m_active_node->get_expanded())
    {
        ImGui::BeginDisabled();
        ImGui::SameLine();
        if (!m_active_node->get_outputs().empty())
        {                
            ImGui::SetCursorPosY(collapsed_pin_y);
            draw_collapsed_pins(false);
        }
        else
        {
            ImGui::Dummy(ImVec2(ImGui::GetStyle().ItemSpacing.x * 2, 0));
        }
        ImGui::EndDisabled();
    }
}

void NodeDrawer::draw_collapsed_pins(bool input)
{
    const std::vector<data::PinInstance*>* pins = nullptr;
    float pos_x = 0.0f;
    if (input)
    {
        pins = &m_active_node->get_inputs();
        pos_x = m_curr_node_left_offset + w::pin_l_offset_rw;        
    }
    else
    {
        pins = &m_active_node->get_outputs();
        pos_x = m_curr_node_left_offset + m_active_node->w_node - w::pin_l_offset_rw - w::ico_circle;
    }

    for (auto pin : *pins)
    {
        ImGui::SetCursorPosX(pos_x);
        draw_pin(pin, true, true);
        ImGui::SameLine();
    }
}

void NodeDrawer::draw_input_pin(data::PinInstance* pin)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw);
    draw_pin(pin, false, false);
    ImGui::SameLine(0.0f, w::pin_r_offset);
    if (!handle_pin_renaming(pin))
        ImGui::TextUnformatted(pin->state().name.c_str(), pin->state().name.c_str() + pin->state().name.size());
}

void NodeDrawer::draw_output_pin(data::PinInstance* pin)
{
    if (!handle_pin_renaming(pin))
        ImGui::TextUnformatted(pin->state().name.c_str(), pin->state().name.c_str() + pin->state().name.size());
    ImGui::SameLine(0, w::pin_r_offset);
    draw_pin(pin, false, false);
}

void NodeDrawer::draw_pin(data::PinInstance* pin, bool collapsed_mode, bool align_to_widget)
{
    ImGui::PushID(&pin->state().pin_color);
#ifdef _DEBUG
    test_id(m_ids_imgui, &pin->state().pin_color);
    test_id(m_ids_nodelib, pin);
#endif
    if (pin->ed())
        ed::BeginPin(pin->ed(), pin->get_object()->input ? ed::PinKind::Input : ed::PinKind::Output);
    else
        pin->ed() = ed::BeginPin(cast::to_ptr(pin), pin->get_object()->input ? ed::PinKind::Input : ed::PinKind::Output);
    draw_pin_shape(pin, collapsed_mode, align_to_widget);
    ed::EndPin();
    ImGui::PopID();
}

void NodeDrawer::draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    if (!pin->get_object()->cag_pin)
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw);
        draw_pin(pin, false, true);
        ImGui::SameLine(0, w::pin_r_offset);
    }
    else
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::pin_r_offset + w::ico_circle);
    draw_widget(pin, widget_name_width, widget_value_width);
}

void NodeDrawer::draw_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    switch (pin->state().type)
    {
    case data::PinType::INT:
    case data::PinType::NUMBER:
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
    case data::PinType::COMBO:
        draw_widget_combo(pin, widget_name_width, widget_value_width);
        break;
    case data::PinType::COLOR:
        draw_widget_color(pin, widget_name_width, widget_value_width);
        break;
    }
}

void NodeDrawer::handle_widget_active_state(data::PinInstance* pin, const primitive* backup /*= nullptr*/)
{
    if (ImGui::IsItemActivated())
        ed::SelectNode(m_active_node->ed());

    if (pin != nullptr)
    {
        if (data::PinType::STRING == pin->state().type && !pin->get_object()->multiline)
        {
            if (ImGui::IsItemActivated())
                m_old_pin_val_pr = pin->state().value;

            const bool active_now = ImGui::IsItemActive();

            if (ImGui::IsItemEdited())
                m_was_edited = pin;

            if (m_editing == pin && !active_now)
            {
                if (m_was_edited == pin)
                {
                    m_wf->action()->set_pin_val(pin, std::move(m_old_pin_val_pr));
                    m_was_edited = nullptr;
                }
            }

            if (active_now)
                m_editing = pin;
        }
        else if (ImGui::IsItemActivated() && backup != nullptr)
            m_old_pin_val_pr = *backup;
        else if (ImGui::IsItemDeactivatedAfterEdit() && pin->state().type != data::PinType::STRING)
            m_wf->action()->set_pin_val(pin, std::move(m_old_pin_val_pr));
    }
}

void NodeDrawer::draw_widget_int(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    const primitive backup = state.value;
    int64_t* val = data::Pin::get_int(state.value);
    const int64_t* step = data::Pin::get_int(obj->step);
    const int64_t* min = data::Pin::get_int(obj->min);
    const int64_t* max = data::Pin::get_int(obj->max);

    ImGui::PushID(&state.value);
#ifdef _DEBUG
    test_id(m_ids_imgui, &state.value);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(nullptr);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->state().links > 0;
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
    else handle_widget_active_state(pin, &backup);

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_uint(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    const primitive backup = state.value;
    uint64_t* val = data::Pin::get_uint(state.value);
    const uint64_t* step = data::Pin::get_uint(obj->step);
    const uint64_t* min = data::Pin::get_uint(obj->min);
    const uint64_t* max = data::Pin::get_uint(obj->max);

    ImGui::PushID(&state.value);
#ifdef _DEBUG
    test_id(m_ids_imgui, &state.value);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(nullptr);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->state().links > 0;
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
    else handle_widget_active_state(pin, &backup);

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_float(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    const primitive backup = state.value;
    double* val = data::Pin::get_float(state.value);
    const double* min = data::Pin::get_float(obj->min);
    const double* max = data::Pin::get_float(obj->max);
    const double* step = data::Pin::get_float(obj->step);

    ImGui::PushID(&state.value);
#ifdef _DEBUG
    test_id(m_ids_imgui, &state.value);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(nullptr);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->state().links > 0;
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
    else handle_widget_active_state(pin, &backup);

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    const primitive backup = state.value;
    bool* val = data::Pin::get_bool(state.value);

    ImGui::PushID(&state.value);
#ifdef _DEBUG
    test_id(m_ids_imgui, &state.value);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(nullptr);
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 0.0f);

    const auto linked = pin->state().links > 0;
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
    else handle_widget_active_state(pin, &backup);

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    auto& placeholder = (obj->placeholder.empty() ? pin->state().name : obj->placeholder);
    std::string* val = data::Pin::get_string(pin->state().value);

    ImGui::PushID(&pin->state().value);
#ifdef _DEBUG
    test_id(m_ids_imgui, &pin->state().value);
#endif

    ImGui::PushItemWidth(widget_name_width + widget_value_width);
    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    ImGuiEx::InputTextWithHint("##value", placeholder.c_str(), val);
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state(pin);
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

    auto& placeholder = (obj->placeholder.empty() ? pin->state().name : obj->placeholder);
    std::string* val = data::Pin::get_string(pin->state().value);

    ImGui::PushID(&pin->state().value);
#ifdef _DEBUG
    test_id(m_ids_imgui, &pin->state().value);
#endif

    ImGui::PushTextWrapPos(0.0f);
    ImGui::SetNextItemWidth(widget_name_width + widget_value_width);
    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    const bool clicked = ImGuiExNe::InputTextWithHint(*m_deffered_action.get(), m_active_node, pin, "##value", placeholder.c_str(), val);
    if (linked) ImGui::EndDisabled();
    ImGui::PopTextWrapPos();

    if (clicked)
        ed::SelectNode(m_active_node->ed(), false);

    bool tt_mod = false;
    if (!val->empty())
        tt_mod = ImGuiEx::TooltipOnLastControl(placeholder.c_str(), placeholder.c_str() + placeholder.size(), ImGuiHoveredFlags_DelayNone, true);

    if (!tt_mod)
        ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());

    ImGui::PopID();
}

void NodeDrawer::draw_widget_combo(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    int64_t* index = data::Pin::get_int(pin->state().value);

    ImGui::PushID(&pin->state().value);
#ifdef _DEBUG
    test_id(m_ids_imgui, &pin->state().value);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(nullptr);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    ImGuiExNe::Combo(*m_deffered_action.get(), m_active_node, pin, "##value", index, obj->combo_items_ptr.data(), cast::to_i64(obj->combo_items_ptr.size()));
    if (linked) ImGui::EndDisabled();

    if (m_deffered_action->active_pin == pin && m_deffered_action->combo.is_changed)
        m_wf->action()->set_pin_val(pin, m_deffered_action->combo.old_value);

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
}

void NodeDrawer::draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    ImGui::PushID(&state.value);
#ifdef _DEBUG
    test_id(m_ids_imgui, &state.value);
#endif

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(nullptr);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const ImVec4 vec4 = ImGui::ColorConvertU32ToFloat4(cast::to_u32(*data::Pin::get_uint(state.value)));
    float colors[4];
    vec4.to_arr(colors);
    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    if (ImGui::ColorEdit4("##value", colors,
        ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop |
        ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoTooltip | (obj->alpha_enabled ? 0 : ImGuiColorEditFlags_NoAlpha)))
    {
        m_wf->action()->set_pin_val(pin, colors);
    }
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state(pin);

    ImGuiEx::TooltipOnLastControl(obj->tooltip.c_str(), obj->tooltip.c_str() + obj->tooltip.size());
    ImGui::PopID();
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
        ed::SelectNode(m_active_node->ed());
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
    for (auto& link : links)
    {
#ifdef _DEBUG
        test_id(m_ids_nodelib, link.get());
#endif
        auto in = link->get_input();
        auto out = link->get_output();
        if (link->ed())
            ed::Link(link->ed(), in->ed(), out->ed(), in->state().pin_color, cast::to_float(g::pOptsManager->wf_link_thickness_load()));
        else
        {
            Log("NEW LINK: " << LogPTR(link.get())); // TODO
            link->ed() = ed::Link(cast::to_ptr(link.get()), in->ed(), out->ed(), in->state().pin_color, cast::to_float(g::pOptsManager->wf_link_thickness_load()));
        }
    }
}

void NodeDrawer::query_new_item(
    const std::function<void(data::PinInstance*)>& unlink_handler,
    const std::function<void(data::PinInstance*, const ImVec2&)>& new_node_handler)
{
    if (ed::BeginCreate())
    {
        ed::PinId start = 0, end = 0;
        ed::PinId pin = 0;
        if (ed::QueryNewLink(&start, &end))
        {
            data::PinInstance* p1 = cast::ptrs<data::PinInstance>(start.Get());
            data::PinInstance* p2 = cast::ptrs<data::PinInstance>(end.Get());
            bool allow = (p1 != p2 && p1 != nullptr && p2 != nullptr);

            if (allow)
            {
                auto linked_node = m_wf->get_node_by_instance(p1->get_parent_node());
                allow = linked_node->get_expanded() && !linked_node->get_object()->is_dummy;
                if (allow)
                {
                    linked_node = m_wf->get_node_by_instance(p2->get_parent_node());
                    allow = linked_node->get_expanded() && !linked_node->get_object()->is_dummy;
                }
            }
            if (allow && data::Link::can_link(p1, p2))
            {
                if (ed::AcceptNewItem())
                {
                    m_wf->action()->multi_action_on("Link");
                    if (p1->state().links > 0 && p1->get_object()->input)
                        unlink_handler(p1);
                    else if (p2->state().links > 0 && p2->get_object()->input)
                        unlink_handler(p2);

                    m_querying_pin = nullptr;
                    m_wf->action()->link(p1, p2);
                    m_wf->action()->multi_action_off();
                }
            }
            else
            {
                ed::RejectNewItem();
            }
        }
        else if (ed::QueryNewNode(&pin))
        {
            m_querying_pin = cast::ptrs<data::PinInstance>(pin.Get());
            if (ed::AcceptNewItem())
            {
                new_node_handler(cast::ptrs<data::PinInstance>(pin.Get()), ed::ScreenToCanvas(ImGui::GetMousePos()));
                m_querying_pin = nullptr;
            }
        }
        ed::EndCreate();
    }
    else
    {
        m_querying_pin = nullptr;
    }
}

void NodeDrawer::handle_deleted()
{
    if (ed::BeginDelete())
    {
        ed::NodeId node_id;
        while (ed::QueryDeletedNode(&node_id))
        {
            if (m_pending_delete.erase(cast::to_void(node_id)) == 0)
            {
                LogAndThrowInternalError(ExcepCategory::API, "Node to delete not found in pending delete list");
            }
            ed::AcceptDeletedItem();
        }

        ed::LinkId link_id;
        while (ed::QueryDeletedLink(&link_id))
        {
            void* ptr = cast::to_void(link_id);
            Log("QUERY DELETE: " << LogPTR(ptr)); // TODO
            if (m_pending_delete.erase(cast::to_void(link_id)) == 0)
            {
                LogAndThrowInternalError(ExcepCategory::API, "Link to delete not found in pending delete list");
            }
            ed::AcceptDeletedItem();
        }

        if (!m_pending_delete.empty())
        {
            LogAndThrowInternalError(ExcepCategory::API, "Not all pending delete items were removed");
        }
    }
    ed::EndDelete();
}

void NodeDrawer::draw_pin_shape(data::PinInstance* pin, bool collapsed_mode, bool align_to_widget)
{
    const bool querying = (m_querying_pin != nullptr);
    const bool compatible = (m_querying_pin == pin || (querying && data::Link::can_link(pin, m_querying_pin)));
    const bool connected = (m_querying_pin == pin || pin->state().links > 0);
    const auto type = (collapsed_mode ? data::PinShape::Circle : data::Pin::get_shape(pin->state().type));
    constexpr const float thickness = 2.0f;
    constexpr const int32_t segments = 12;
    const auto draw_list = ImGui::GetWindowDrawList();
    const float alpha = (!querying || compatible ? 1.0f : 0.4f);
    const auto color = (collapsed_mode ? IM_COL32_WHITE : ((pin->state().pin_color & 0x00FFFFFF) | ((ImU32)(alpha * 255.0f) << 24)));
    const float radius = (!querying ? 4.0f : (compatible ? 5.0f : 3.0f));
    const float diameter = radius * 2.0f;
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    if (align_to_widget)
        cursor_pos.y += ImGui::GetTextLineHeightWithSpacing() / 2;
    else
        cursor_pos.y += w::ico_circle / 2;
    const ImVec2 pin_center = ImVec2(cursor_pos.x + w::ico_circle / 2, cursor_pos.y);

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
        if (m_resizing_grip_positions[idx].GetHeight() <= 0)
            continue;

        ImGui::SetCursorScreenPos(m_resizing_grip_positions[idx].Min);

        ImGui::PushID(idx);
        ImGui::InvisibleButton("##grip", ImVec2(m_grip_area_sz, m_resizing_grip_positions[idx].GetHeight()));
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

bool NodeDrawer::get_bg_menu_active(bool& rclick, bool& dbl_click) const
{
    rclick = dbl_click = false;
    if (ed::ShowBackgroundContextMenu())
        rclick = true;
    else if (ed::IsBackgroundDoubleClicked())
        dbl_click = true;
    return (rclick || dbl_click);
}

ImVec2 NodeDrawer::get_mouse_pos() const
{
    auto prev_context = ed::GetCurrentEditor();
    if (nullptr == prev_context)
        ed::SetCurrentEditor(m_context);
    const auto pos = ed::ScreenToCanvas(ImGui::GetMousePos());
    if (nullptr == prev_context)
        ed::SetCurrentEditor(nullptr);
    return pos;
}

data::PinInstance* NodeDrawer::get_hovered_pin() const
{
    return cast::ptrs<data::PinInstance>((std::uintptr_t)ed::GetHoveredPin());
}

data::NodeInstance* NodeDrawer::get_hovered_node() const
{
    return cast::ptrs<data::NodeInstance>((std::uintptr_t)ed::GetHoveredNode());
}

data::Link* NodeDrawer::get_hovered_link() const
{
    return cast::ptrs<data::Link>((std::uintptr_t)ed::GetHoveredLink());
}

std::set<data::NodeInstance*> NodeDrawer::get_selected_nodes() const
{
    const auto count = ed::GetSelectedNodesCount();
    if (0 == count)
        return {};

    std::vector<ed::NodeId> nodes;
    nodes.resize(count);
    ed::GetSelectedNodes(nodes.data(), count);

    std::set<data::NodeInstance*> ret;
    for (auto id : nodes)
    {
        ret.insert(cast::ptrs<data::NodeInstance>((std::uintptr_t)id));
    }
    return ret;
}

bool NodeDrawer::is_node_selected(const EdType& node) const
{
    return ed::IsNodeSelected(node);
}

void NodeDrawer::select_node(const EdType& node, bool append_to_selection) const
{
    if (node)
        ed::SelectNode(node, append_to_selection);
    else
        ed::ClearSelection();
}

std::set<data::Link*> NodeDrawer::get_selected_links() const
{
    std::vector<ed::LinkId> links;
    const auto count = ed::GetSelectedLinksCount();
    links.resize(count);
    ed::GetSelectedLinks(links.data(), count);

    std::set<data::Link*> ret;
    for (auto id : links)
    {
        ret.insert(cast::ptrs<data::Link>((std::uintptr_t)id));
    }
    return ret;
}

bool NodeDrawer::is_link_selected(const EdType& link) const
{
    return ed::IsLinkSelected(link);
}

void NodeDrawer::select_link(const EdType& link, bool append_to_selection) const
{
    if (link)
        ed::SelectLink(link, append_to_selection);
    else
        ed::ClearSelection();
}

void NodeDrawer::clear_selection()
{
    ed::ClearSelection();
}

std::set<data::NodeInstance*> NodeDrawer::get_nodes_in_group(data::NodeInstance* group, const std::vector<std::unique_ptr<data::NodeInstance>>& all_nodes) const
{
    std::set<data::NodeInstance*> ret;
    const float group_capt_height = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;

    for (auto& node : all_nodes)
    {
        if (node->m_rc.Min.x > group->m_rc.Min.x && node->m_rc.Min.y > group->m_rc.Min.y + group_capt_height &&
            node->m_rc.Min.x + node->m_rc.Max.x < group->m_rc.Min.x + group->m_rc.Max.x &&
            node->m_rc.Min.y + node->m_rc.Max.y < group->m_rc.Min.y + group->m_rc.Max.y)
        {
            ret.insert(node.get());
        }
    }

    return ret;
}

void NodeDrawer::pin_node(const EdType& node, bool pin) const
{
    ed::SetNodeDraggable(node, !pin);
}

void NodeDrawer::rename_node(data::NodeInstance* node)
{
    m_renaming_node = node;
    m_renaming_pin = nullptr;
    m_renaming_buff = node->get_name();
    m_need_renaming_focus = true;
}

void NodeDrawer::rename_pin(data::PinInstance* pin)
{
    m_renaming_node = nullptr;
    m_renaming_pin = pin;
    m_renaming_buff = pin->state().name;
    m_need_renaming_focus = true;
}

ImRect NodeDrawer::calc_group_rect(const ImRect& node_rc) const
{
    return ImRect(ImVec2(node_rc.Min.x - ImGui::GetStyle().ItemSpacing.x * 2, node_rc.Min.y - ImGui::GetTextLineHeight() * 2 - ImGui::GetStyle().ItemSpacing.y * 2),
        ImVec2(node_rc.Max.x + ImGui::GetStyle().ItemSpacing.x * 4, node_rc.Max.y + ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y * 2));
}

ImRect NodeDrawer::fit_group_rect(const data::NodeInstance* group, const std::set<data::NodeInstance*>& nodes) const
{
    if (nodes.empty())
        return {};

    auto rect = ImRect(FLT_MAX, FLT_MAX, -FLT_MIN, -FLT_MIN);
    for (auto node : nodes)
    {
        rect.Min.x = std::min(group->m_rc.Min.x, node->m_rc.Min.x);
        rect.Min.y = std::min(group->m_rc.Min.y, node->m_rc.Min.y);
        rect.Max.x = std::max(group->m_rc.Max.x, node->m_rc.Min.x + node->m_rc.Max.x);
        rect.Max.y = std::max(group->m_rc.Max.y, node->m_rc.Min.y + node->m_rc.Max.y);
    }

    rect.Max.x = group->m_rc.GetWidth() + ImGui::GetStyle().ItemSpacing.x * 4;
    rect.Max.y = group->m_rc.GetHeight() + ImGui::GetStyle().ItemSpacing.y * 4;

    rect.Min.x -= ImGui::GetStyle().ItemSpacing.x * 2;
    rect.Min.y -= ImGui::GetTextLineHeight() * 3 - ImGui::GetStyle().ItemSpacing.y * 4;

    return rect;
}

void NodeDrawer::delete_node(const data::NodeInstance* node)
{
    if (node->ed())
    {
        auto editor = ed::GetCurrentEditor();
        if (nullptr == editor)
            ed::SetCurrentEditor(m_context);

        if (m_context != nullptr)
        {
            ed::DeleteNode(node->ed());
            m_pending_delete.insert(cast::to_void(node));
        }

        if (nullptr == editor)
            ed::SetCurrentEditor(nullptr);
    }
    else
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Node id is null");
    }
}

void NodeDrawer::delete_link(const data::Link* link)
{
    auto editor = ed::GetCurrentEditor();
    if (nullptr == editor)
        ed::SetCurrentEditor(m_context);

    if (m_context != nullptr)
    {
        bool result = false;
        Log("DELETE LINK: " << LogPTR(link)); // TODO
        if (link->ed())
            result = ed::DeleteLink(link->ed());
        else
            result = ed::DeleteLink(cast::to_ptr(link));
        if (result)
            m_pending_delete.insert(cast::to_void(link));
    }

    if (nullptr == editor)
        ed::SetCurrentEditor(nullptr);    
}

float NodeDrawer::get_zoom() const
{
    return ed::GetCurrentZoom();
}

void NodeDrawer::set_zoom(float zoom)
{
    m_zoom = zoom;
    m_zoom_set = false;
}

const ImRect& NodeDrawer::get_viewport_rect() const
{
    return ed::GetCurrentView();
}

void NodeDrawer::set_viewport_rect(const ImRect& rect)
{
    m_view_rect = rect;
    m_view_set = false;
}

ImVec2 NodeDrawer::get_visible_pos(std::optional<ImVec2> add_offset) const
{    
    const ImVec2 add_pt = add_offset.has_value() ? add_offset.value() : ImVec2(0, 0);
    if (nullptr == m_context)
        return add_pt;

    auto prev_context = ed::GetCurrentEditor();
    ed::SetCurrentEditor(m_context);

    const auto screen_top_left = get_viewport_rect();
    const ImVec2 target_screen_pos = ImVec2(screen_top_left.Min.x + add_pt.x, screen_top_left.Min.y + add_pt.y);

    ed::SetCurrentEditor(prev_context);
    return target_screen_pos;
}

bool NodeDrawer::handle_pin_renaming(data::PinInstance* curr_pin)
{
    if (m_renaming_pin == curr_pin)
    {
        if (m_need_renaming_focus)
        {
            ImGui::SetKeyboardFocusHere();
            m_need_renaming_focus = false;
        }
        ImGui::SetNextItemWidth(m_active_node->get_object()->w_pin_in_max);
        if (ImGuiEx::InputText("##pinrename", &m_renaming_buff, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue) ||
            ImGui::IsItemDeactivatedAfterEdit())
        {
            m_wf->action()->set_pin_name(m_renaming_pin, m_renaming_buff);
            m_renaming_buff.clear();            
            m_renaming_pin = nullptr;
            m_renaming_node = nullptr;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_renaming_pin = nullptr;
            m_renaming_node = nullptr;
        }
        return true;
    }
    return false;
}

void NodeDrawer::handle_node_collapsing(data::NodeInstance* node)
{
    g::pFlow->txt_panel_delete(m_wf->get_id(), node);
    if (m_deffered_action->active_node == node)
    {
        m_deffered_action->combo.is_active = false;
        m_deffered_action->ml_input.is_active = false;
        m_deffered_action->active_node = nullptr;
        m_deffered_action->active_pin = nullptr;
    }
    if (m_renaming_node == node)
    {
        m_wf->action()->set_node_name(m_renaming_node, m_renaming_buff);
        m_renaming_buff.clear();
        m_renaming_node = nullptr;
        m_renaming_pin = nullptr;
    }
    for (auto pin : node->get_inputs())
    {
        if (pin == m_renaming_pin)
        {
            m_wf->action()->set_pin_name(m_renaming_pin, m_renaming_buff);
            m_renaming_buff.clear();            
            m_renaming_pin = nullptr;
            m_renaming_node = nullptr;
        }
        else if (pin == m_editing && pin == m_was_edited)
        {
            m_wf->action()->set_pin_val(pin, std::move(m_old_pin_val_pr));
            m_was_edited = nullptr;
            m_editing = nullptr;
        }
    }
    for (auto pin : node->get_outputs())
    {
        if (pin == m_renaming_pin)
        {
            m_wf->action()->set_pin_name(m_renaming_pin, m_renaming_buff);
            m_renaming_buff.clear();            
            m_renaming_pin = nullptr;
            m_renaming_node = nullptr;
            break;
        }
    }
}

} // namespace ui
