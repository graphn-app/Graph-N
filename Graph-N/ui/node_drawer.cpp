#include "ui/node_drawer.h"
#include "ui/helpers/imgui_ex_ne.h"
#include "ui/graph-n_flow.h"
#include "ui/custom_widgets/custom_widget.h"
#include "core/managers/settings_manager.h"
#include "core/managers/history_manager.h"
#include "core/workflow/node.h"
#include "core/workflow/pin.h"
#include "core/workflow/link.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/custom_nodes/cni_reroute.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "imgui_node_editor.h"
#include "main.h"

/*
NODE


|<------------------------------------------------------0------------------------------------------>|
|<------------------------------------------------------1/21--------------------------------------->|         ---->|
| 19
|                                     [             node name           ]                           |              |
|<---------------2------------------->|<--------------3/4-------------->|<-----------2------------->|         ---->| 
| 19
|     [       pin_ico    ]     [in_pin_name][    center      ][out_pin_name]     [ pin_ico    ]     |              |
|<-5->|<------6/13------>|<-7->|<---8/9--->|[<-----10------->|<---11/12--->|<-7->|<---6/13--->|<-5->|         ---->|
| 20
|     [    widget_ico    ]     [  widget_name       ][     widget_value    ]                        |              |
|<-5->|<------13-------->|<-7->|<-------14/15------>||<---------16/17----->|<-7->|<----6/13-->|<-5->|         ---->|

|     [                                            separator                                  |     |
|<-5->|<------13-------->|<-7->|<-----------------18-----------------------|<-7->|<----6/13-->|<-5->|


----------------------------------------------------------------------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------------------------------------------------------------------
##  | what                   | equal to                                                         | variable                          | when
----------------------------------------------------------------------------------------------------------------------------------------------------------------
3   | node name              | CalcTextSize(Node::name)                                         | Node::w_name                      | on parse
5   | pin left offset        | 4.0                                                              | w::pin_l_offset                   | app start
6   | icon width             | CalcTextSize(ICON_CIRCLE)                                        | w::ico_circle                     | app start
7   | pin right offset       | 4.0                                                              | w::pin_r_offset                   | app start
8   | input pin name         | CalcTextSize(Pin::name))                                         | Pin::w_name                       | parse
11  | output pin name        | CalcTextSize(Pin::name))                                         | Pin::w_name                       | parse
13  | icon width             | CalcTextSize(ICON_EYE)                                           | w::ico_eye                        | app start
14  | widget name            | CalcTextSize(Pin::name))                                         | Pin::w_name                       | parse
16  | widget value           | CalcTextSize(Pin::value))                                        | Pin::w_value                      | parse
19  | vertical spacing       | 4.0                                                              | w::v_spacing                      | app start
20  | vertical spacing       | (19)*2                                                           |                                   | app start
----------------------------------------------------------------------------------------------------------------------------------------------------------------
9   | max input pin name     | max(!wigdet(8))                                                  | Node::w_pin_in_max                | first instance create
12  | max output pin name    | max(11)                                                          | Node::w_pin_out_max               | first instance create
15  | max widget name        | max(widget(14) + 2 * ImGui::GetStyle().ItemSpacing.x)            | Node::w_widget_name_max           | first instance create
17  | max widget value       | max(16)                                                          | Node::w_widget_value_max          | first instance create
21  | node ideal fit         | = (1 ideal)                                                      | Node::w_fit                       | first instance create
----------------------------------------------------------------------------------------------------------------------------------------------------------------
0   | available width        |                                                                  |                                   | every frame
1   | node width             | (5)*2 + (13)*2 + (7)*2 + (15) + (17)                             | NodeInstance::w_node              | every frame
2   | node name l/r margins  | (0)/2 - (4)/2                                                    | NodeInstance::w_node_name_off     | every frame
4   | node name              | CalcTextSize(Node::name)                                         | NodeInstance::w_name              | on node rename
10  | central zone           | (15) + (17) - (9) - (12)                                         | NodeInstance::w_center            | every frame
18  | hidden items separator | (0) - (5)*2 + (13)*2 + (7)*2                                     | NodeInstance:::w_separator        | every frame
----------------------------------------------------------------------------------------------------------------------------------------------------------------

- pin icon (16), widget ico (13) - take max depend on: tooltip, inspection, wf

- empty node:       (no in pins, no out pins, no wdg):     - (1) = (100)
- widgets only:     (no in pins, no out pins):             - (1) = (5)*2 + (13)*2 + (7)*2 + (15) + (17)
- input only:       (no out pins, no wdg):                 - (10) = 50, (1) = (5)*2 + (13)*2 + (7)*2 + (9) + (10)
- output only:      (no in pins, no wdg):                  - (10) = 50, (1) = (5)*2 + (13)*2 + (7)*2 + (10) + (12)
- wdg and out:      (no in pins):                          - (1) = (5)*2 + (13)*2 + (7)*2 + (15) + (17), (10) = (15) + (17) - (12)
- wdg and in:       (no out pins):                         - (1) = (5)*2 + (13)*2 + (7)*2 + (15) + (17), (10) = (15) + (17) - (9)
- in and out:       (no wdg):                              - (10) = 10, (1) = (5)*2 + (13)*2 + (7)*2 + (9) + (10) + (12)

- if (0) < (21):           - (15) = ((0) - (5)*2 - (13)*2 - (7)*2) / 3, but >= 0
- (17) = ((0) - (5)*2 - (13)*2 - (7)*2) / 3 * 2, but >= 0
- (10) = (15) + (17) - (9) - (12), but >= 0
- (9) = ((15) + (17) / 2), but >= 0
- (12) = ((15) + (17) / 2), but >= 0
- (1) = (0)
- if (0) > (21):           - (15) = fixed
- (17) = (0) - (5)*2 - (13)*2 - (7)*2 - (15)
- (9) = fixed
- (12) = fixed
- (10) = (0) - (5)*2 - (13)*2 - (7)*2 - (9) - (12)
- (1) = (0)

*/

namespace ui
{

namespace ed = ax::NodeEditor;

NodeDrawer::NodeDrawer(data::Workflow* wf) noexcept
    : m_wf(wf)
    , m_clr_node_header_text(g::pOptsManager->color_load(AppColor::NodeHeaderText))
    , m_clr_node_pin_text(g::pOptsManager->color_load(AppColor::NodePinText))    
{
}

NodeDrawer::~NodeDrawer() noexcept
{
    if (m_context != nullptr)
        ed::DestroyEditor(m_context);
}

void NodeDrawer::init(blob::Blob&& state)
{
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
            m_navigate_to_content = true;
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

void NodeDrawer::free() noexcept
{
    m_deffered_action.reset();
    m_state->clear();

    if (m_context != nullptr)
        ed::DestroyEditor(m_context);
    m_context = nullptr;
}

void NodeDrawer::locate_node(data::NodeInstance* node) noexcept
{
    LogRAssert(node, "Zero ptr", );
    const auto* prev_context = ed::GetCurrentEditor();
    if (nullptr == prev_context)
        ed::SetCurrentEditor(m_context);

    ed::ClearSelection();
    ed::SelectNode(node->ed());
    ed::NavigateToContent();
    ed::NavigateToSelection(false, 0.5f);

    if (nullptr == prev_context)
        ed::SetCurrentEditor(nullptr);
}

void NodeDrawer::locate_selected() noexcept
{
    if (ed::GetSelectedNodesCount() > 0)
        ed::NavigateToSelection(true, 0.5f);
    else
        ed::NavigateToContent();
}

data::PinInstance* NodeDrawer::get_ml_widget_active() const noexcept
{
    if (nullptr == m_deffered_action->active_pin)
        return nullptr;
    return m_deffered_action->active_pin;
}

void NodeDrawer::pre_draw_all(const char* id, size_t /*nodes_count*/, bool first_frame)
{
    //m_resizing_grip_positions.clear(); // TODO:FIX resizing
    //m_resizing_grip_positions.resize(nodes_count);

    m_active_tt = nullptr;
    m_hovered_widget = nullptr;
    m_first_frame = first_frame;

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


    auto& io = ImGui::GetIO();
    m_mouse_wheel = io.MouseWheel;
    if (io.KeyShift) 
        io.MouseWheel *= 0.1f; 

    ed::Begin(id, ImVec2(0, 0));

    m_zoom = ed::GetCurrentZoom();
    if (m_zoom <= ZOOMED_MAX) [[likely]]    m_zoomed_visibility = ZoomedVisibility::Max;
    else if (m_zoom <= ZOOMED_FADE)         m_zoomed_visibility = ZoomedVisibility::Fade;
    else if (m_zoom <= ZOOMED_MEDIUM)       m_zoomed_visibility = ZoomedVisibility::Medium;
    else if (m_zoom <= ZOOMED_LOW)          m_zoomed_visibility = ZoomedVisibility::Low;
    else                                    m_zoomed_visibility = ZoomedVisibility::None;
}

void NodeDrawer::post_draw_all(
    const std::function<bool()>& init_menu_handler,
    const std::function<bool()>& show_menu_handler,
    const std::function<bool()>& move_handler,
    const std::function<bool()>& hotkey_handler)
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
    if (!canvas_busy)
        canvas_busy = hotkey_handler();

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

    if (m_active_tt != nullptr && *m_active_tt != 0)
        tooltip_suspended([this]() { return m_active_tt; });

    if (m_navigate_to_content)
    {
        m_navigate_to_content = false;
        ed::NavigateToContent();
    }

    ed::SetCurrentEditor(nullptr);

    ImGui::GetIO().MouseWheel = m_mouse_wheel;
}

void NodeDrawer::pre_draw_node(data::NodeInstance* node, bool execution_failed)
{
    LogRAssert(node, "Zero ptr", );
    LogRAssert(nullptr == m_active_node, "Active node must be zero ptr", );

    if (node->flags().has(NodeInstFlags::Group))
    {
        ed::PushStyleColor(ed::StyleColor_GroupBg, ImVec4(0, 0, 0, 0));
        ed::PushStyleColor(ed::StyleColor_GroupBorder, ImVec4(0, 0, 0, 0));
        ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0, 0, 0, 0));
    }
    else
    {
        if (node->obj_flags().has(NodeObjFlags::Dummy) || execution_failed)
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

    if (node->get_mode_mute())
    {
        ed::PushStyleColor(ed::StyleColor_NodeBorder, ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::NodeBorderMuted)));
        ed::PushStyleColor(ed::StyleColor_NodeBg, ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::NodeBgMuted)));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * m_mode_alpha);
    }
    else if (node->get_mode_bypass())
    {
        ed::PushStyleColor(ed::StyleColor_NodeBorder, ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::NodeBorderBypassed)));
        ed::PushStyleColor(ed::StyleColor_NodeBg, ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::NodeBgBypassed)));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * m_mode_alpha);
    }

    if (node->flags().has_not(NodeInstFlags::PosSet))
    {
        ed::SetNodePosition(node->ed(), node->m_rc.Min);
        node->flags().add(NodeInstFlags::PosSet);
    }
    if (node->flags().has_and_has_not(NodeInstFlags::Group, NodeInstFlags::SizeSet))
    {
        ed::SetGroupSize(node->ed(), node->m_rc.Max);
        node->flags().add(NodeInstFlags::SizeSet);
    }

    ed::BeginNode(node->ed());

    if (node->flags().has_not(NodeInstFlags::PinnedSet))
    {
        ed::SetNodeDraggable(node->ed(), node->flags().has_not(NodeInstFlags::Pinned));
        node->flags().add(NodeInstFlags::PinnedSet);
    }

    const bool deferred_node_active = m_deffered_action->active_node == node;
    if (deferred_node_active)
    {
        if (ed::HasSelectionChanged())
            m_deffered_action->active_node = nullptr;
    }
}

void NodeDrawer::post_draw_node(data::NodeInstance* node, bool execution_failed) noexcept
{
    LogRAssert(node, "Zero ptr", );
    LogRAssert(nullptr == m_active_node, "Active node must be zero ptr", );
    ed::EndNode();

    node->m_rc.Min = ed::GetNodePosition(node->ed());

    if (node->flags().has_not(NodeInstFlags::IgnoreSizeOnFrameEnd))
        node->m_rc.Max = ed::GetNodeSize(node->ed());
    node->flags().remove(NodeInstFlags::IgnoreSizeOnFrameEnd);

    if (node->flags().has(NodeInstFlags::Group))
        ed::PopStyleColor(3);
    else if (node->obj_flags().has(NodeObjFlags::Dummy) || execution_failed)
        ed::PopStyleColor(6);
    else
        ed::PopStyleColor();

    if (!node->get_mode_always())
    {
        ed::PopStyleColor(2);
        ImGui::PopStyleVar();
    }
}

void NodeDrawer::draw_node(data::NodeInstance* node, float node_w, size_t /*node_idx*/, const std::set<std::string>* failed_pins)
{
    LogRAssert(node, "Zero ptr", );
    m_active_node = node;

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

    if (m_active_node->flags().has_not(NodeInstFlags::SizeSet) && m_active_node->m_rc.Max.x > 0)
    {
        node_w = m_active_node->m_rc.Max.x;
        m_active_node->flags().add(NodeInstFlags::SizeSet);
    }
    node_w = m_active_node->recalc_node_width(node_w, false, false);

    //if (node_size.x > 0 && node_size.y > 0) // TODO:FIX resizing
    //{
    //    m_resizing_w[node_idx] = (m_active_node->get_pinned() || !m_active_node->get_expanded() ? -1 : node_w);
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
    if (m_active_node->flags().has_not(NodeInstFlags::Expanded))
    {
        m_active_node = nullptr;
        return;
    }

    if (zoomed_skip_links()) [[unlikely]]
    {
        ImGui::Dummy(ImVec2(node_w, m_active_node->m_last_all_widgets_h));
        m_active_node = nullptr;
        return;
    }

    ImGui::Dummy(ImVec2(m_active_node->w_node, ImGui::GetTextLineHeight() * 0.5f));
    auto y_off = ImGui::GetCursorPosY();

    // --- PINS - INPUT ---

    ImGui::PushStyleColor(ImGuiCol_Text, m_clr_node_pin_text);

    bool draw_pins = true;
    if (m_active_node->flags().has(NodeInstFlags::RerouteSplitted)) [[unlikely]]
    {
        const auto reroute_splitting_pin = cast::strict<data::CNI_Reroute>(m_active_node)->get_splitter_input();
        if (reroute_splitting_pin != nullptr)
        {
            ImGui::PushID(reroute_splitting_pin);
            draw_input_pin(reroute_splitting_pin, false);
            ImGui::PopID();
            draw_pins = false;
        }

    }
    if (draw_pins) [[likely]]
    {
        const auto& inputs = m_active_node->get_inputs();
        for (auto pin : inputs)
        {
            if (pin->flags().has_not(PinInstFlags::Hidden) && !pin->get_object()->is_widget())
            {
                ImGui::PushID(pin);
                const bool failed = (failed_pins != nullptr && failed_pins->count(pin->get_object()->def_state.name));
                draw_input_pin(pin, failed);
                ImGui::PopID();
            }
        }
    }

    const auto last_in_off_y = ImGui::GetCursorPosY();

    // --- SPACER ---

    if (!m_active_node->get_outputs().empty())
    {
        ImGui::SetCursorPosY(y_off);
        if (m_active_node->w_center > 0.0f)
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::ico_max + w::pin_r_offset + m_active_node->w_pin_in + m_active_node->w_center);
    }

    // --- PINS - OUTPUT ---

    draw_pins = true;
    if (m_active_node->flags().has(NodeInstFlags::RerouteSplitted)) [[unlikely]]
    {
        const auto reroute_splitting_pin = cast::strict<data::CNI_Reroute>(m_active_node)->get_splitter_output();
        if (reroute_splitting_pin != nullptr)
        {
            ImGui::PushID(reroute_splitting_pin);
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::ico_max + w::pin_r_offset + m_active_node->w_pin_in + m_active_node->w_center + m_active_node->w_pin_out);
            draw_output_pin(reroute_splitting_pin);
            ImGui::PopID();
            draw_pins = false;
        }

    }
    if (draw_pins) [[likely]]
    {
        const auto& output = m_active_node->get_outputs();
        for (auto pin : output)
        {
            if (pin->flags().has(PinInstFlags::Hidden))
                continue;

            ImGui::PushID(pin);
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::ico_max + w::pin_r_offset + m_active_node->w_pin_in + m_active_node->w_center +
                m_active_node->w_pin_out - pin->state().w_name);

            draw_output_pin(pin);
            ImGui::PopID();
        }
    }

    ImGui::PopStyleColor();

    if (m_active_node->flags().has_not(NodeInstFlags::HasWidgets))
        ImGui::SetCursorPosY(std::max(last_in_off_y, ImGui::GetCursorPosY()));
    ImGui::Dummy(ImVec2(m_active_node->w_node, w::v_spacing * 2));

    if (zoomed_draw_all()) [[likely]]
        m_active_node->m_last_pins_h = ImGui::GetCursorPosY() - y_off;
    y_off = ImGui::GetCursorPosY();

    // TODO:FIX resizing
   // m_resizing_grip_positions[node_idx].Min.y -= ImGui::GetCursorPos().y - m_curr_node_top_offset + ImGui::GetTextLineHeight();

    // --- WIDGETS ---

    if (m_active_node->flags().has(NodeInstFlags::HasWidgets))
    {
        ImGui::SetCursorPosY(std::max(last_in_off_y, ImGui::GetCursorPosY()));

        size_t advanced = 0;
        const auto& inputs = m_active_node->get_inputs();
        for (auto pin : inputs)
        {
            auto obj = pin->get_object();

            if (obj->flags.has(PinObjFlags::Advanced))
            {
                advanced++;
                if (pin->state().links > 0)
                    m_active_node->flags().add(NodeInstFlags::ShowHidden);
            }
            else if (obj->is_widget())
            {
                ImGui::PushID(pin);
                draw_input_widget(pin, m_active_node->w_widget_name, m_active_node->w_widget_value);
                ImGui::PopID();
            }
        }
        if (advanced > 0)
        {
            bool temp_open = false;
            bool hidden = m_active_node->flags().has(NodeInstFlags::ShowHidden);
            draw_widget_separator(w::pin_l_offset_rw + w::ico_max + w::pin_r_offset, hidden, temp_open);
            m_active_node->flags().add_remove_if(NodeInstFlags::ShowHidden, hidden);
            if (hidden || temp_open) [[unlikely]]
            {
                for (auto pin : inputs)
                {
                    auto obj = pin->get_object();
                    if (obj->flags.has(PinObjFlags::Advanced))
                    {
                        ImGui::PushID(pin);
                        draw_input_widget(pin, m_active_node->w_widget_name, m_active_node->w_widget_value);
                        ImGui::PopID();
                    }
                }
            }
        }
        else
            m_active_node->flags().remove(NodeInstFlags::ShowHidden);
    }

    if (m_active_node->obj_flags().has(NodeObjFlags::CustomWidget))
    {
        if (zoomed_skip_widget_content()) [[unlikely]]
        {
            ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::pin_r_offset + w::ico_circle);
            draw_zoomed_low_box(m_active_node->w_widget_name + m_active_node->w_widget_value, m_active_node->m_last_custom_widgets_h);
        }
        else
        {
            if (zoomed_draw_fade())
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, get_zoomed_fade_alpha());

            const auto y = ImGui::GetCursorPosY();
            g::pFlow->custom_widgets().draw_custom_widget(m_active_node, m_curr_node_left_offset + w::pin_l_offset_rw + w::pin_r_offset + w::ico_circle, m_active_node->w_widget_name + m_active_node->w_widget_value);
            m_active_node->m_last_custom_widgets_h = ImGui::GetCursorPosY() - y;

            if (zoomed_draw_fade())
                ImGui::PopStyleVar();
        }
    }

    if (m_active_node->flags().has(NodeInstFlags::HasWidgets) && m_active_node->obj_flags().has_not(NodeObjFlags::CustomWidget))
        ImGui::Dummy(ImVec2(m_active_node->w_node, 10.0f));

    if (zoomed_draw_all()) [[likely]]
        m_active_node->m_last_all_widgets_h = ImGui::GetCursorPosY() - y_off;
    m_active_node = nullptr;
}

void NodeDrawer::draw_group(data::NodeInstance* node, size_t /*node_idx*/)
{
    m_active_node = node;

    m_curr_node_left_offset = ImGui::GetCursorScreenPos().x;
    m_curr_node_top_offset = ImGui::GetCursorScreenPos().y;

    const auto& style = ed::GetStyle();

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

        if (m_active_node != m_renaming_node && !zoomed_skip_captions()) [[likely]]
        {
            ImGui::SetCursorPosX(text_lt.x);
            ImGui::PushStyleColor(ImGuiCol_Text, m_active_node->m_clr_group_capt);
            ImGui::TextUnformatted(m_active_node->get_name().c_str(), m_active_node->get_name().c_str() + m_active_node->get_name().size());

            //width = m_active_node->m_rc.Max.x - ImGui::GetStyle().ItemSpacing.x * 4; // TODO:FIX name too long: use draw_list->AddText
            //const char* pend = nullptr;
            //const char* pbeg = ImGuiEx::cut_text_with_ellipsis(m_active_node->get_name(), width, pend);
            //ImGui::TextUnformatted(pbeg, pend);

            ImGui::PopStyleColor();
            if (zoomed_allow_renaming() && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
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
            g::pFlow->txt_panel_update(m_wf->get_id(), m_active_node);
            m_renaming_buff.clear();
            m_renaming_buff.shrink_to_fit();
            m_renaming_node = nullptr;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_renaming_node = nullptr;
        }
    }

    ed::Group(m_active_node->m_rc.Max);
    m_active_node = nullptr;
}

void NodeDrawer::draw_node_header()
{
    auto draw_list = ImGui::GetWindowDrawList();
    if (draw_list != nullptr)
    {
        const auto& style = ed::GetStyle();

        //const ImU32 bg_color = (m_active_node->m_clr_custom ? m_active_node->m_clr_head : m_defclr_node_header_bg);
        const float header_h = ImGui::GetTextLineHeight() * 1.5f - (m_active_node->flags().has(NodeInstFlags::Expanded) ? 0 : style.NodeBorderWidth);

        draw_list->AddRectFilled(
            ImVec2(m_curr_node_left_offset - style.NodePadding.x + style.NodeBorderWidth, m_curr_node_top_offset - style.NodePadding.y + style.NodeBorderWidth),
            ImVec2(m_curr_node_left_offset + style.NodePadding.x + m_active_node->w_node - style.NodeBorderWidth, m_curr_node_top_offset + header_h),
            m_active_node->m_clr_head,
            style.NodeRounding,
            (m_active_node->flags().has(NodeInstFlags::Expanded) ? ImDrawFlags_RoundCornersTop : ImDrawFlags_RoundCornersAll)
        );
    }

    const float collapsed_pin_y = ImGui::GetCursorPosY();
    if (m_active_node->flags().has_not(NodeInstFlags::Expanded) && !m_active_node->get_inputs().empty())
    {
        if (zoomed_skip_pin_shape()) [[unlikely]]
            ImGui::Dummy(ImVec2(w::ico_circle, w::ico_circle));
        else
            draw_collapsed_pins(true);
        ImGui::SameLine();
    }

    if (m_active_node != m_renaming_node)
    {
        if (!zoomed_skip_captions()) [[likely]]
        {
            ImGui::SetCursorPosX(m_curr_node_left_offset + m_active_node->w_node_name_off);
            ImGui::PushStyleColor(ImGuiCol_Text, m_active_node->obj_flags().has(NodeObjFlags::Dummy) ? g::pOptsManager->color_load(AppColor::DummyNode) : m_clr_node_header_text);
            ImGui::TextUnformatted(m_active_node->get_name().c_str(), m_active_node->get_name().c_str() + m_active_node->get_name().size());
            ImGui::PopStyleColor();

            if (zoomed_allow_renaming() && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (m_active_node->has_object() && m_active_node->obj_flags().has(NodeObjFlags::CanRename))
                {
                    m_renaming_node = m_active_node;
                    m_renaming_buff = m_active_node->get_name();
                    m_need_renaming_focus = true;
                }
            }

            if (m_active_node->flags().has(NodeInstFlags::Pinned))
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeHeaderIcon));
                ImGui::TextUnformatted(" " ICON_LC_PIN);
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) m_active_tt = "Node is pinned to the canvas.";
            }
            if (m_active_node->get_mode_bypass())
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeHeaderIcon));
                ImGui::TextUnformatted(" " ICON_LC_SKIP_FORWARD);
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) m_active_tt = "Node is bypassed. Inputs pass through to outputs.";
            }
            else if (!m_active_node->get_mode_always())
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeHeaderIcon));
                ImGui::TextUnformatted(" " ICON_LC_VOLUME_X);
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) m_active_tt = "Node is muted. It will not be processed.";
            }
        }
        else
        {
            ImGui::Dummy({});
        }
    }
    else if (m_renaming_node == m_active_node)
    {
        if (m_need_renaming_focus)
        {
            ImGui::SetKeyboardFocusHere();
            m_need_renaming_focus = false;
        }
        if (m_active_node->flags().has(NodeInstFlags::Expanded))
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
            g::pFlow->txt_panel_update(m_wf->get_id(), m_active_node);
            m_renaming_buff.clear();
            m_renaming_buff.shrink_to_fit();
            m_renaming_node = nullptr;
            m_renaming_pin = nullptr;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_renaming_buff.clear();
            m_renaming_buff.shrink_to_fit();
            m_renaming_node = nullptr;
            m_renaming_pin = nullptr;
        }
    }

    if (m_active_node->flags().has_not(NodeInstFlags::Expanded))
    {
        ImGui::BeginDisabled();
        ImGui::SameLine();
        if (!m_active_node->get_outputs().empty())
        {                
            ImGui::SetCursorPosY(collapsed_pin_y);
            if (zoomed_skip_pin_shape()) [[unlikely]]
            {
                ImGui::Dummy(ImVec2(w::ico_circle, w::ico_circle));
                ImGui::SameLine();
            }
            else
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
    float pos_x = 0.0f;
    if (input)
        pos_x = m_curr_node_left_offset + w::pin_l_offset_rw;        
    else
        pos_x = m_curr_node_left_offset + m_active_node->w_node - w::pin_l_offset_rw - w::ico_circle;
    ImGui::SetCursorPosX(pos_x);

    data::PinInstance* reroute_splitted_pin = nullptr;
    if (m_active_node->flags().has(NodeInstFlags::RerouteSplitted)) [[unlikely]]
    {
        if (input)
            reroute_splitted_pin = cast::strict<data::CNI_Reroute>(m_active_node)->get_splitter_input();
        else
            reroute_splitted_pin = cast::strict<data::CNI_Reroute>(m_active_node)->get_splitter_output();
    }

    if (reroute_splitted_pin != nullptr) [[unlikely]]
    {
        ImGui::SetCursorPosX(pos_x);
        draw_pin(reroute_splitted_pin, true, true, false);
        ImGui::SameLine();
    }
    else
    {
        const std::vector<data::PinInstance*>* pins = nullptr;
        if (input)
            pins = &m_active_node->get_inputs();
        else
            pins = &m_active_node->get_outputs();

        for (auto pin : *pins)
        {
            ImGui::SetCursorPosX(pos_x);
            draw_pin(pin, true, true, false);
            ImGui::SameLine();
        }
    }
}

void NodeDrawer::draw_input_pin(data::PinInstance* pin, bool failed_pin)
{
    ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw);
    draw_pin(pin, false, false, failed_pin);
    ImGui::SameLine(0.0f, w::pin_r_offset);
    if (!handle_pin_renaming(pin))
    {
        if (zoomed_skip_pin_names()) [[unlikely]]
            ImGui::Dummy(ImVec2(pin->state().w_name, 0.0f));
        else
            ImGui::TextUnformatted(pin->state().name.c_str(), pin->state().name.c_str() + pin->state().name.size());
    }
}

void NodeDrawer::draw_output_pin(data::PinInstance* pin)
{
    if (!handle_pin_renaming(pin))
    {
        if (zoomed_skip_pin_names()) [[unlikely]]
            ImGui::Dummy(ImVec2(pin->state().w_name, 0.0f));
        else
            ImGui::TextUnformatted(pin->state().name.c_str(), pin->state().name.c_str() + pin->state().name.size());
    }
    ImGui::SameLine(0, w::pin_r_offset);
    draw_pin(pin, false, false, false);
}

void NodeDrawer::draw_pin(data::PinInstance* pin, bool collapsed_mode, bool align_to_widget, bool failed_pin)
{
    ImGui::PushID(0x5F329AF2);
    ImGui::PushID(pin);
    ed::BeginPin(pin->ed(), pin->obj_flags().has(PinObjFlags::Input) ? ed::PinKind::Input : ed::PinKind::Output);
    if (zoomed_skip_pin_shape())
        ImGui::Dummy(ImVec2(w::ico_circle, w::ico_circle));
    else
        draw_pin_shape(pin, collapsed_mode, align_to_widget, failed_pin);
    ed::EndPin();
    ImGui::PopID(2);
}

void NodeDrawer::draw_input_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    if (pin->obj_flags().has(PinObjFlags::CanLink))
    {
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw);
        draw_pin(pin, false, true, false);
        ImGui::SameLine(0, w::pin_r_offset);
    }
    else
        ImGui::SetCursorPosX(m_curr_node_left_offset + w::pin_l_offset_rw + w::pin_r_offset + w::ico_circle);
    draw_widget(pin, widget_name_width, widget_value_width);
}

void NodeDrawer::draw_widget(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    if (zoomed_skip_widget_content()) [[unlikely]]
    {
        draw_zoomed_medium_control(widget_name_width + widget_value_width, PinType::STRING == pin->state().type && pin->is_multiline());
        return;
    }

    if (zoomed_draw_fade())
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, get_zoomed_fade_alpha());

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
        if (pin->is_multiline())
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

    if (zoomed_draw_fade())
        ImGui::PopStyleVar();
}

void NodeDrawer::handle_widget_active_state(data::PinInstance* pin, const Primitive* backup /*= nullptr*/)
{
    if (ImGui::IsItemHovered())
        m_hovered_widget = pin;
    if (ImGui::IsItemActivated())
        ed::SelectNode(m_active_node->ed());

    if (backup != nullptr)
    {
        if (PinType::STRING == pin->state().type && !pin->is_multiline())
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
        else if (ImGui::IsItemDeactivatedAfterEdit() && pin->state().type != PinType::STRING)
            m_wf->action()->set_pin_val(pin, std::move(m_old_pin_val_pr));
    }
}

void NodeDrawer::draw_widget_int(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    const Primitive backup = state.value;
    int64_t* val = state.value.get_int_ref();
    LogRAssert(val, "Zero val", );
    const int64_t step = obj->def_state.step.get_int();
    const int64_t min = obj->def_state.min.get_int();
    const int64_t max = obj->def_state.max.get_int();

    ImGui::PushID(0xF3C4D86E);
    ImGui::PushID(pin);

    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(pin);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
    default:
        ImGuiEx::InputIntSmart("##value", val, step, min, max, obj->precision, obj->precision);
        break;
    case WidgetInput::Drag:
        //ImGuiEx::DragIntSmart("##value", val, min, max, obj->precision.c_str());
    case WidgetInput::Slider:
        ImGuiEx::SliderIntSmart("##value", val, min, max, obj->precision.c_str());                
        break;
    }
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state(pin, &backup);

    tooltip_suspended(obj->tooltip);
    draw_cag_helper(pin);

    ImGui::PopID(2);
}

void NodeDrawer::draw_widget_float(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    const Primitive backup = state.value;
    double* val = state.value.get_float_ref();
    LogRAssert(val, "Zero val", );
    const double min = obj->def_state.min.get_float();
    const double max = obj->def_state.max.get_float();
    const double step = obj->def_state.step.get_float();

    ImGui::PushID(0x6322BBE5);
    ImGui::PushID(pin);
    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(pin);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
    default:
        ImGuiEx::InputDoubleSmart("##value", val, step, min, max, obj->precision, obj->precision);
        break;
    case WidgetInput::Drag:
        //ImGuiEx::DragDoubleSmart("##value", val, min, max, obj->precision.c_str());
    case WidgetInput::Slider:
        ImGuiEx::SliderDoubleSmart("##value", val, min, max, obj->precision.c_str());        
        break;
    }
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state(pin, &backup);

    tooltip_suspended(obj->tooltip);
    draw_cag_helper(pin);

    ImGui::PopID(2);
}

void NodeDrawer::draw_widget_number(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    static_assert(std::same_as<number_t, double>, "Update this function if number_t changes");

    const Primitive backup = state.value;

    number* num_ptr = state.value.get_number_ref();
    LogRAssert(num_ptr, "Zero ptr", );

    double* val = num_ptr->underlying_type_ref();
    LogRAssert(val, "Zero val", );

    const double min = obj->def_state.min.get_number().underlying_type();
    const double max = obj->def_state.max.get_number().underlying_type();
    const double step = obj->def_state.step.get_number().underlying_type();

    ImGui::PushID(0x746F8133);
    ImGui::PushID(pin);
    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(pin);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    switch (g::pOptsManager->wf_widget_input_numbers_load())
    {
    case WidgetInput::Direct:
    default:
        ImGuiEx::InputDoubleSmart("##value", val, step, min, max, obj->precision, obj->precision);
        break;
    case WidgetInput::Drag:
        //ImGuiEx::DragDoubleSmart("##value", val, min, max, obj->precision.c_str());
    case WidgetInput::Slider:
        ImGuiEx::SliderDoubleSmart("##value", val, min, max, obj->precision.c_str());        
        break;
    }
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state(pin, &backup);

    tooltip_suspended(obj->tooltip);
    draw_cag_helper(pin);

    ImGui::PopID(2);
}

void NodeDrawer::draw_widget_bool(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    auto& state = pin->state();

    const Primitive backup = state.value;
    bool* val = state.value.get_bool_ref();
    LogRAssert(val, "Zero val", );

    ImGui::PushID(0x8FF6FCC5);
    ImGui::PushID(pin);
    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(pin);
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

    tooltip_suspended(obj->tooltip);
    ImGui::PopID(2);
}

void NodeDrawer::draw_widget_str(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();

    const auto& placeholder = (obj->placeholder.empty() ? pin->state().name : obj->placeholder);
    std::string* val = pin->state().value.get_string_ref();
    LogRAssert(val, "Zero val", );

    ImGui::PushID(0x9A5218DB);
    ImGui::PushID(pin);
    ImGui::PushItemWidth(widget_name_width + widget_value_width);
    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    ImGuiEx::InputTextWithHint("##value", placeholder.c_str(), val);
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state(pin);
    ImGui::PopItemWidth();

    bool tt_mod = false;
    if (!val->empty())
        tt_mod = tooltip_suspended(placeholder);

    if (!tt_mod)
        tooltip_suspended(obj->tooltip);

    ImGui::PopID(2);
}

void NodeDrawer::draw_widget_str_ml(data::PinInstance* pin, float widget_name_width, float widget_value_width) noexcept
{
    auto obj = pin->get_object();

    const auto& placeholder = (obj->placeholder.empty() ? pin->state().name : obj->placeholder);
    std::string* val = pin->state().value.get_string_ref();
    LogRAssert(val, "Zero val", );

    ImGui::PushID(0x9AC25C3D);
    ImGui::PushID(pin);
    ImGui::PushTextWrapPos(0.0f);
    ImGui::SetNextItemWidth(widget_name_width + widget_value_width);
    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    const bool clicked = ImGuiExNe::InputTextWithHint(*m_deffered_action.get(), m_active_node, pin, "##value", placeholder.c_str(), val);
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state(pin);
    ImGui::PopTextWrapPos();

    if (clicked)
        ed::SelectNode(m_active_node->ed(), false);

    bool tt_mod = false;
    if (!val->empty())
        tt_mod = tooltip_suspended(placeholder);

    if (!tt_mod)
        tooltip_suspended(obj->tooltip);

    ImGui::PopID(2);
}

void NodeDrawer::draw_widget_combo(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    int64_t* index = pin->state().value.get_int_ref();
    LogRAssert(index, "Zero val", );

    ImGui::PushID(0xAE809F86);
    ImGui::PushID(pin);
    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(pin);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    ImGuiExNe::Combo(*m_deffered_action.get(), m_active_node, pin, "##value", index, obj->combo_items_ptr.data(), cast::to_i64(obj->combo_items_ptr.size()));
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state(pin);

    if (m_deffered_action->active_pin == pin && m_deffered_action->combo.is_changed)
        m_wf->action()->set_pin_val(pin, m_deffered_action->combo.old_value);

    tooltip_suspended(obj->tooltip);
    draw_cag_helper(pin);

    ImGui::PopID(2);
}

void NodeDrawer::draw_widget_color(data::PinInstance* pin, float widget_name_width, float widget_value_width)
{
    auto obj = pin->get_object();
    const auto& state = pin->state();

    ImGui::PushID(0xB9C9528C);
    ImGui::PushID(pin);
    ImGui::SetNextItemWidth(widget_name_width);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGuiEx::InputText("##name", &pin->state().name, ImGuiInputTextFlags_ReadOnly);
    handle_widget_active_state(pin);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetNextItemWidth(widget_value_width);

    const ImVec4 vec4 = ImGui::ColorConvertU32ToFloat4(cast::to_u32(state.value.get_int()));
    float colors[4];
    vec4.to_arr(colors);
    const auto linked = pin->state().links > 0;
    if (linked) ImGui::BeginDisabled();
    if (ImGui::ColorEdit4("##value", colors,
        ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop |
        ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoTooltip | (obj->flags.has(PinObjFlags::ClrAlphaEnabled) ? 0 : ImGuiColorEditFlags_NoAlpha)))
    {
        m_wf->action()->set_pin_val(pin, colors);
    }
    if (linked) ImGui::EndDisabled();
    else handle_widget_active_state(pin);

    tooltip_suspended(obj->tooltip);
    ImGui::PopID(2);
}

void NodeDrawer::draw_widget_separator(float offset, bool& opened, bool& temp_open) noexcept
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

void NodeDrawer::draw_cag_helper(const data::PinInstance* pin) const noexcept
{
    if (ControlAfterGenerate::Fixed == pin->state().cag_val) [[likely]]
        return;

    const char* helper = nullptr;
    if (ControlAfterGenerate::Increment == pin->state().cag_val)
        helper = ICON_LC_TRENDING_UP;
    else if (ControlAfterGenerate::Decrement == pin->state().cag_val)
        helper = ICON_LC_TRENDING_DOWN;
    else if (ControlAfterGenerate::Randomize == pin->state().cag_val)
        helper = ICON_LC_SHUFFLE;
    else
        LogRError("Invalid CAG value", );

    ImGui::PushFont(g::pMain->get_small_font());
    ImGui::SameLine(0, 0);
    ImGui::TextUnformatted(helper);
    ImGui::PopFont();

    const auto get_value = [](const Primitive& p)
    {
        return
            std::visit([](const auto& v) -> std::string
            {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, int64_t>)
                    return fmt::format(FMT_STRING("{}"), v);
                else if constexpr (std::is_same_v<T, double>)
                    return fmt::format(FMT_STRING("{:.2f}"), v);
                else if constexpr (std::is_same_v<T, number>)
                    return fmt::format(FMT_STRING("{:.2f}"), v.underlying_type());
                else
                    return std::string("");
            }, p.value());
    };

    static fmt::memory_buffer tooltip;
    const auto tooltip_handler = [pin, &get_value]()
    {
        tooltip.clear();
        if (ControlAfterGenerate::Increment == pin->state().cag_val || ControlAfterGenerate::Decrement == pin->state().cag_val)
        {
            if (PinType::COMBO == pin->state().type)
            {
                fmt::format_to(std::back_inserter(tooltip),
                    FMT_STRING("Mode: {}"),
                    (ControlAfterGenerate::Increment == pin->state().cag_val ? "Increment" : "Decrement"));
            }
            else
            {
                fmt::format_to(std::back_inserter(tooltip),
                    FMT_STRING("Mode: {}\nStep: {}\nLimit: {} - {}"),
                    (ControlAfterGenerate::Increment == pin->state().cag_val ? "Increment" : "Decrement"),
                    get_value(pin->state().step),
                    get_value(pin->state().min), get_value(pin->state().max));
            }
        }
        else
        {
            fmt::format_to(std::back_inserter(tooltip),
                FMT_STRING("Mode: Randomize\nLimit: {} - {}"),
                get_value(pin->state().min), get_value(pin->state().max));
        }
        tooltip.push_back(0);
        return tooltip.data();
    };

    tooltip_suspended(tooltip_handler());
}

void NodeDrawer::draw_links(const std::vector<std::unique_ptr<data::Link>>& links)
{
    if (zoomed_skip_links()) [[unlikely]]
        return;

    for (auto& link : links)
    {
        auto in = link->get_input();
        auto out = link->get_output();
        ed::Link(link->ed(), in->ed(), out->ed(), in->state().pin_color, link->get_link_thickness_mult() * g::pOptsManager->wf_link_thickness_load());
    }
}

void NodeDrawer::query_new_item(
    const std::function<void(data::PinInstance*)>& unlink_handler,
    const std::function<void(data::PinInstance*, const ImVec2&)>& new_node_handler)
{
    if (ed::BeginCreate())
    {
        uint64_t start = 0, end = 0;
        uint64_t pin = 0;
        if (ed::QueryNewLink(start, end) && start > 0 && end > 0)
        {
            data::PinInstance* p1 = m_wf->ed_get_pin(start);
            data::PinInstance* p2 = m_wf->ed_get_pin(end);
            bool allow = (p1 != p2 && p1 != nullptr && p2 != nullptr && p1->flags().has_not(PinInstFlags::RerouteSplittingPin) && p2->flags().has_not(PinInstFlags::RerouteSplittingPin));

            if (allow)
            {
                auto linked_node = m_wf->get_node_by_instance(p1->get_parent_node());
                allow = linked_node->flags().has(NodeInstFlags::Expanded) && linked_node->obj_flags().has_not(NodeObjFlags::Dummy);
                if (allow)
                {
                    linked_node = m_wf->get_node_by_instance(p2->get_parent_node());
                    allow = linked_node->flags().has(NodeInstFlags::Expanded) && linked_node->obj_flags().has_not(NodeObjFlags::Dummy);
                }
            }
            if (allow && data::Link::can_link(p1, p2))
            {
                if (ed::AcceptNewItem())
                {
                    m_wf->action()->multi_action_on("Link");
                    if (p1->state().links > 0 && p1->obj_flags().has(PinObjFlags::Input))
                        unlink_handler(p1);
                    else if (p2->state().links > 0 && p2->obj_flags().has(PinObjFlags::Input))
                        unlink_handler(p2);

                    m_querying_pin = nullptr;
                    m_wf->action()->link(p1, p2);
                    m_wf->action()->multi_action_off();

                    auto node = m_wf->get_node_by_instance(p1->get_parent_node());
                    node->reset_width();
                    if (p1->obj_flags().has(PinObjFlags::Input))
                        g::pFlow->txt_panel_delete(m_wf->get_id(), node, p1);

                    node = m_wf->get_node_by_instance(p2->get_parent_node());
                    node->reset_width();
                    if (p2->obj_flags().has(PinObjFlags::Input))
                        g::pFlow->txt_panel_delete(m_wf->get_id(), node, p2);
                }
            }
            else
            {
                ed::RejectNewItem();
            }
        }
        else if (ed::QueryNewNode(pin) && pin > 0)
        {
            m_querying_pin = m_wf->ed_get_pin(pin);
            if (ed::AcceptNewItem())
            {
                if (m_querying_pin != nullptr && m_querying_pin->get_object()->parent_node->flags.has_not(NodeObjFlags::Dummy) && m_querying_pin->flags().has_not(PinInstFlags::RerouteSplittingPin))
                    new_node_handler(m_querying_pin, ed::ScreenToCanvas(ImGui::GetMousePos()));
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

void NodeDrawer::handle_deleted() noexcept
{
    if (ed::BeginDelete())
    {
        uint64_t node_id = 0;
        while (ed::QueryDeletedNode(node_id) && node_id > 0)
        {
            ed::AcceptDeletedItem();
        }

        uint64_t link_id = 0;
        while (ed::QueryDeletedLink(link_id) && link_id > 0)
        {
            ed::AcceptDeletedItem();
        }
    }
    ed::EndDelete();
}

void NodeDrawer::draw_pin_shape(data::PinInstance* pin, bool collapsed_mode, bool align_to_widget, bool failed_pin)
{
    LogRAssert(pin, "Zero ptr", );

    const bool querying = (m_querying_pin != nullptr);
#if 0
    const bool compatible = true;
#else
    const bool compatible = (m_querying_pin == pin || (querying && data::Link::can_link(pin, m_querying_pin)));
#endif
    const bool connected = (m_querying_pin == pin || pin->state().links > 0);
    const auto type = (collapsed_mode && pin->flags().has_not(PinInstFlags::RerouteSplittingPin) ? PinShape::Circle : data::Pin::get_shape(pin->state().type));
    const float thickness = failed_pin ? 2.5f : 2.0f;
    constexpr const int32_t segments = 12;
    const auto draw_list = ImGui::GetWindowDrawList();
    const float alpha = (!querying || compatible ? 1.0f : 0.4f);
    const auto color = (collapsed_mode ? IM_COL32_WHITE : ((pin->state().pin_color & 0x00FFFFFF) | ((ImU32)(alpha * 255.0f) << 24)));
    const float radius = (!querying ? 4.0f : (compatible ? 5.0f : 3.0f));
    //const float diameter = radius * 2.0f;
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    if (align_to_widget)
        cursor_pos.y += ImGui::GetTextLineHeightWithSpacing() / 2;
    else
        cursor_pos.y += w::ico_circle / 2;
    const ImVec2 pin_center = ImVec2(cursor_pos.x + w::ico_circle / 2, cursor_pos.y);

    if (failed_pin)
    {
        const ImU32 error_bg_color = IM_COL32(255, 40, 40, (ImU32)(alpha * 90.0f));
        const ImU32 error_fg_color = IM_COL32(255, 40, 40, (ImU32)(alpha * 255.0f));
        const float error_radius = radius + 3.5f;

        draw_list->AddCircleFilled(pin_center, error_radius, error_bg_color, segments);
        draw_list->AddCircle(pin_center, error_radius, error_fg_color, segments, 1.5f);
    }

    switch (type)
    {
    case PinShape::Diamond:
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
    case PinShape::Hexagon:
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
                    const float angle = cast::to_float(i) * 1.047f;
                    draw_list->PathLineTo(ImVec2(pin_center.x + r * cosf(angle), pin_center.y + r * sinf(angle)));
                }
                draw_list->PathStroke(color, ImDrawFlags_Closed, thickness);
            }
        }
        break;
    case PinShape::SemiCircle:
        {
            bool in = pin->obj_flags().has(PinObjFlags::Input);
            if (pin->flags().has(PinInstFlags::RerouteSplittingPin))
                in = !in;
            if (in)
                draw_list->PathArcTo(pin_center, radius, IM_PI * 1.5f, IM_PI * 2.5f, segments / 2);
            else
                draw_list->PathArcTo(pin_center, radius, IM_PI * 0.5f, IM_PI * 1.5f, segments / 2);
            draw_list->PathFillConvex(color);
        }
        break;
    case PinShape::Square:
        {
            if (connected)
                draw_list->AddRectFilled(pin_center - ImVec2(radius, radius) * 0.85f, pin_center + ImVec2(radius, radius) * 0.85f, color, radius * 0.2f);
            else
                draw_list->AddRect(pin_center - ImVec2(radius, radius) * 0.85f, pin_center + ImVec2(radius, radius) * 0.85f, color, radius * 0.2f, ImDrawFlags_None, thickness);
        }
        break;
    case PinShape::Triangle:
        {
            const float r = radius * 1.1f;
            constexpr float angle_offset = 0.0f;
            const ImVec2 p1 = ImVec2(pin_center.x + r * cosf(angle_offset), pin_center.y + r * sinf(angle_offset));
            const ImVec2 p2 = ImVec2(pin_center.x + r * cosf(angle_offset + 2.094f), pin_center.y + r * sinf(angle_offset + 2.094f));
            const ImVec2 p3 = ImVec2(pin_center.x + r * cosf(angle_offset + 4.188f), pin_center.y + r * sinf(angle_offset + 4.188f));
            if (connected)
                draw_list->AddTriangleFilled(p1, p2, p3, color);
            else
                draw_list->AddTriangle(p1, p2, p3, color, thickness);
        }
        break;
    default: // fallthrough to default Circle
        LogError("Unhandled pin shape, pin type " << cast::to_i32(type));
        [[fallthrough]];
    case PinShape::Circle:
        {
            if (connected)
                draw_list->AddCircleFilled(pin_center, radius, color, segments);
            else
                draw_list->AddCircle(pin_center, radius, color, segments, thickness);
            break;
        }
    }

    ImGui::Dummy(ImVec2(w::ico_circle, w::ico_circle));
    tooltip_suspended(pin->get_object()->tooltip, m_clr_node_pin_text);
}

bool NodeDrawer::handle_node_resize() noexcept
{
    if (m_resizing_grip_positions.empty())
        return false;

    m_now_resizing = -1;
    const ImVec2 old_pos = ImGui::GetCursorScreenPos();

    ImGui::PushID(0xE42BFB7E);
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
                const ImVec2 p1 = ed::ScreenToCanvas(ImVec2(0, 0));
                const ImVec2 p2 = ed::ScreenToCanvas(ImGui::GetIO().MouseDelta);
                const ImVec2 canvas_delta = ImVec2(p2.x - p1.x, p2.y - p1.y);
                m_resizing_w[m_now_resizing] += canvas_delta.x;
                m_resizing_w[m_now_resizing] = std::max(0.0f, m_resizing_w[m_now_resizing]);
            }
        }
    }
    ImGui::PopID();

    ImGui::SetCursorScreenPos(old_pos);
    return m_now_resizing != -1;
}

bool NodeDrawer::get_bg_menu_active(bool& rclick, bool& dbl_click) const noexcept
{
    rclick = dbl_click = false;
    if (ed::ShowBackgroundContextMenu())
        rclick = true;
    else if (ed::IsBackgroundDoubleClicked())
        dbl_click = true;
    return (rclick || dbl_click);
}

ImVec2 NodeDrawer::get_mouse_pos(const ImVec2* convert_pos /*= nullptr*/) const noexcept
{
    const auto* prev_context = ed::GetCurrentEditor();
    if (nullptr == prev_context)
        ed::SetCurrentEditor(m_context);

    ImVec2 pos;
    if (convert_pos != nullptr)
        pos = ed::ScreenToCanvas(*convert_pos);
    else
        pos = ed::ScreenToCanvas(ImGui::GetMousePos());

    if (nullptr == prev_context)
        ed::SetCurrentEditor(nullptr);
    return pos;
}

data::PinInstance* NodeDrawer::get_hovered_pin() const
{
    return m_wf->ed_get_pin(ed::GetHoveredPin());
}

data::NodeInstance* NodeDrawer::get_hovered_node() const
{
    return m_wf->ed_get_node(ed::GetHoveredNode());
}

data::Link* NodeDrawer::get_hovered_link() const
{
    return m_wf->ed_get_link(ed::GetHoveredLink());
}

std::set<data::NodeInstance*> NodeDrawer::get_selected_nodes(bool reroutes_as_single_node) const
{
    const auto count = ed::GetSelectedNodesCount();
    if (0 == count)
        return {};

    const std::set<uint64_t> nodes = ed::GetSelectedNodes();

    std::set<data::NodeInstance*> ret;
    for (auto id : nodes)
    {
        auto node = m_wf->ed_get_node(id);
        LogCAssert(node, "Zero selected node");
        if (reroutes_as_single_node && node->flags().has(NodeInstFlags::RerouteSplitted_R))
        {
            node = cast::strict<data::CNI_Reroute>(node)->get_splitted_base();
            LogCAssert(node, "Zero splitted node");
        }

        ret.insert(node);
    }
    return ret;
}

bool NodeDrawer::is_node_selected(const EdType& node) const noexcept
{
    return ed::IsNodeSelected(node);
}

void NodeDrawer::select_node(const EdType& node, bool append_to_selection) const noexcept
{
    LogRAssert(node.has_id(), "Node has no id", );
    ed::SelectNode(node, append_to_selection);
}

void NodeDrawer::clear_selection() noexcept
{
    ed::ClearSelection();
}

std::set<data::Link*> NodeDrawer::get_selected_links() const
{
    const std::set<uint64_t> links = ed::GetSelectedLinks();

    std::set<data::Link*> ret;
    for (auto id : links)
{
        ret.insert(m_wf->ed_get_link(id));
    }
    return ret;
}

bool NodeDrawer::is_link_selected(const EdType& link) const noexcept
{
    return ed::IsLinkSelected(link);
}

void NodeDrawer::select_link(const EdType& link, bool append_to_selection) const noexcept
{
    if (link.has_id())
        ed::SelectLink(link, append_to_selection);
    else
        ed::ClearSelection();
}

std::set<data::NodeInstance*> NodeDrawer::get_nodes_in_group(const data::NodeInstance* group, const std::vector<std::unique_ptr<data::NodeInstance>>& all_nodes) const
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

void NodeDrawer::pin_node(const EdType& node, bool pin) const noexcept
{
    ed::SetNodeDraggable(node, !pin);
}

void NodeDrawer::rename_node(data::NodeInstance* node) noexcept
{
    LogRAssert(node, "Zero ptr", );
    m_renaming_node = node;
    m_renaming_pin = nullptr;
    m_renaming_buff = node->get_name();
    m_need_renaming_focus = true;
}

void NodeDrawer::rename_pin(data::PinInstance* pin) noexcept
{
    LogRAssert(pin, "Zero ptr", );
    m_renaming_node = nullptr;
    m_renaming_pin = pin;
    m_renaming_buff = pin->state().name;
    m_need_renaming_focus = true;
}

ImRect NodeDrawer::calc_group_rect(const ImRect& node_rc) const noexcept
{
    return ImRect(ImVec2(node_rc.Min.x - ImGui::GetStyle().ItemSpacing.x * 2, node_rc.Min.y - ImGui::GetTextLineHeight() * 2 - ImGui::GetStyle().ItemSpacing.y * 2),
        ImVec2(node_rc.Max.x + ImGui::GetStyle().ItemSpacing.x * 4, node_rc.Max.y + ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y * 2));
}

ImRect NodeDrawer::fit_group_rect(const data::NodeInstance* /*group*/, const std::set<data::NodeInstance*>& nodes) const noexcept
{
    if (nodes.empty())
        return {};

    auto rect = ImRect(FLT_MAX, FLT_MAX, -FLT_MIN, -FLT_MIN);
    for (auto node : nodes)
    {
        rect.Min.x = std::min(rect.Min.x, node->m_rc.Min.x);
        rect.Min.y = std::min(rect.Min.y, node->m_rc.Min.y);
        rect.Max.x = std::max(rect.Max.x, node->m_rc.Min.x + node->m_rc.Max.x);
        rect.Max.y = std::max(rect.Max.y, node->m_rc.Min.y + node->m_rc.Max.y);
    }

    rect.Min.x -= ImGui::GetStyle().ItemSpacing.x * 2;
    rect.Min.y -= ImGui::GetTextLineHeight() * 3 - ImGui::GetStyle().ItemSpacing.y * 4;

    rect.Max.x = rect.Max.x - rect.Min.x;
    rect.Max.y = rect.Max.y - rect.Min.y;

    rect.Max.x += ImGui::GetStyle().ItemSpacing.x * 4;
    rect.Max.y += ImGui::GetStyle().ItemSpacing.y * 4;

    return rect;
}

void NodeDrawer::delete_node(data::NodeInstance* node)
{
    LogRAssert(node, "Zero ptr", );
    if (nullptr == m_context) return;

    const auto* editor = ed::GetCurrentEditor();
    if (nullptr == editor)
        ed::SetCurrentEditor(m_context);

    if (m_context != nullptr)
        ed::DeleteNode(node->ed());

    if (nullptr == editor)
        ed::SetCurrentEditor(nullptr);
}

void NodeDrawer::delete_link(data::Link* link)
{
    LogRAssert(link, "Zero ptr", );
    if (nullptr == m_context) return;

    const auto* editor = ed::GetCurrentEditor();
    if (nullptr == editor)
        ed::SetCurrentEditor(m_context);

    if (m_context != nullptr)
        ed::DeleteLink(link->ed());

    if (nullptr == editor)
        ed::SetCurrentEditor(nullptr);    
}

const ImRect& NodeDrawer::get_viewport_rect() const noexcept
{
    return ed::GetCurrentView();
}

void NodeDrawer::set_viewport_rect(const ImRect& rect) noexcept
{
    m_view_rect = rect;
    m_view_set = false;
}

ImVec2 NodeDrawer::get_visible_pos(std::optional<ImVec2> add_offset) const noexcept
{    
    const ImVec2 add_pt = add_offset.value_or(ImVec2(0, 0));
    if (nullptr == m_context)
        return add_pt;

    auto prev_context = ed::GetCurrentEditor();
    ed::SetCurrentEditor(m_context);

    const auto screen_top_left = get_viewport_rect();
    const ImVec2 target_screen_pos = ImVec2(screen_top_left.Min.x + add_pt.x, screen_top_left.Min.y + add_pt.y);

    ed::SetCurrentEditor(prev_context);
    return target_screen_pos;
}

bool NodeDrawer::handle_pin_renaming(const data::PinInstance* curr_pin)
{
    if (m_renaming_pin == curr_pin)
    {
        if (m_need_renaming_focus)
        {
            ImGui::SetKeyboardFocusHere();
            m_need_renaming_focus = false;
        }
        ImGui::SetNextItemWidth(m_renaming_pin->obj_flags().has(PinObjFlags::Input) ? m_active_node->get_object()->w_pin_in_max : m_active_node->get_object()->w_pin_out_max);
        if (ImGuiEx::InputText("##pinrename", &m_renaming_buff, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue) ||
            ImGui::IsItemDeactivatedAfterEdit())
        {
            m_wf->action()->set_pin_name(m_renaming_pin, m_renaming_buff);
            m_renaming_buff.clear();
            m_renaming_buff.shrink_to_fit();
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

void NodeDrawer::handle_node_collapsing(const data::NodeInstance* node)
{
    //g::pFlow->txt_panel_delete(m_wf->get_id(), node);
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
        g::pFlow->txt_panel_update(m_wf->get_id(), m_active_node);
        m_renaming_buff.clear();
        m_renaming_buff.shrink_to_fit();
        m_renaming_node = nullptr;
        m_renaming_pin = nullptr;
    }
    for (auto pin : node->get_inputs())
    {
        if (pin == m_renaming_pin)
        {
            m_wf->action()->set_pin_name(m_renaming_pin, m_renaming_buff);
            m_renaming_buff.clear();
            m_renaming_buff.shrink_to_fit();
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
            m_renaming_buff.shrink_to_fit();
            m_renaming_pin = nullptr;
            m_renaming_node = nullptr;
            break;
        }
    }
}

float NodeDrawer::get_zoomed_fade_alpha() const noexcept
{
    const float fade_range = ZOOMED_FADE - ZOOMED_MAX;
    float alpha = 1.0f - ((m_zoom - ZOOMED_MAX) / fade_range);
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    return std::max(0.2f, alpha);
}

void NodeDrawer::draw_zoomed_medium_control(float width, bool multiline) const noexcept
{
    const float frame_height = (multiline ? ImGui::GetFrameHeight() * WIDGET_STRING_ML_H : ImGui::GetFrameHeight());
    draw_zoomed_low_box(width, frame_height);
}

void NodeDrawer::draw_zoomed_low_box(float width, float height) const noexcept
{
    const ImVec2 p_min = ImGui::GetCursorScreenPos();
    const ImVec2 p_max = ImVec2(p_min.x + width, p_min.y + height);
    ImGui::RenderFrame(p_min, p_max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, ImGui::GetStyle().FrameRounding);
    ImGui::Dummy(ImVec2(width, height));
}

bool NodeDrawer::tooltip_suspended(const std::function<const char*()>& tooltip, ImU32 pop_color /*= 0*/) const noexcept
{
    bool ret = false;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
    {
        ax::NodeEditor::Suspend(); 
        if (ImGui::BeginTooltip())
        {
            if (pop_color != 0)
                ImGui::PopStyleColor();
            ImGui::PushTextWrapPos(300.0f);
            ImGui::TextUnformatted(tooltip());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
            if (pop_color != 0)
                ImGui::PushStyleColor(ImGuiCol_Text, pop_color);
            ret = true;
        }
        ax::NodeEditor::Resume(); 
    }
    return ret;
}

bool NodeDrawer::tooltip_suspended(const std::string& tooltip, ImU32 pop_color /*= 0*/) const noexcept
{
    bool ret = false;
    if (!tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
    {
        ed::Suspend(); 
        if (ImGui::BeginTooltip())
        {
            if (pop_color != 0)
                ImGui::PopStyleColor();
            ImGui::PushTextWrapPos(300.0f);
            ImGui::TextUnformatted(tooltip.c_str(), tooltip.c_str() + tooltip.size());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
            if (pop_color != 0)
                ImGui::PushStyleColor(ImGuiCol_Text, pop_color);
            ret = true;
        }
        ed::Resume(); 
    }
    return ret;
}

} // namespace ui
