#include "ui/panels/activity_bar.h"
#include "ui/graph-n_flow.h"
#include "ui/helpers/font_guard.h"
#include "core/managers/settings_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "main.h"

namespace ui
{

ActivityBar::ActivityBar() noexcept = default;
ActivityBar::~ActivityBar() noexcept = default;

void ActivityBar::register_panels(const std::vector<DlgObj>& panels)
{
    std::ranges::for_each(panels, [this](DlgObj id) { m_panels.push_back({ id, Direction::None, "" }); });

    g::pHotkeysManager->add_listener(this);
    handle_hotkeys_changed();

    m_clr_bg = g::pOptsManager->color_load(AppColor::ActivityBarBg);
    m_clr_inactive = g::pOptsManager->color_load(AppColor::ActivityBarInactive);

    m_clr_active = g::pOptsManager->color_load(AppColor::ActivityBarActive);
    m_clr_hover = ImGuiEx::increase_brightness(m_clr_active);
    m_clr_pressed = ImGuiEx::increase_brightness(m_clr_hover);

    m_clr_restart_inactive = g::pOptsManager->color_load(AppColor::RestartButton);
    m_clr_restart_hover = ImGuiEx::increase_brightness(m_clr_restart_inactive);
    m_clr_restart_pressed = ImGuiEx::increase_brightness(m_clr_restart_hover);
}

void ActivityBar::handle_hotkeys_changed()
{
    const auto set_hotkey_tooltip = [](const char* hotkey, std::string& target)
    {
        if (nullptr == hotkey)
            target.clear();
        else
            target = std::string(" | ") + hotkey;
    };

    for (auto& pair : m_panels)
    {
        KeyTarget target;
        switch (std::get<0>(pair))
        {
        case DlgObj::PanelNodes:            target = KeyTarget::PanelOpen_Nodes;       break;
        case DlgObj::PanelNodeInspector:    target = KeyTarget::PanelOpen_Inspector;   break;
        case DlgObj::PanelStats:            target = KeyTarget::PanelOpen_Stats;       break;
        case DlgObj::PanelWfList:           target = KeyTarget::PanelOpen_Projects;    break;
        case DlgObj::PanelTemplates:        target = KeyTarget::PanelOpen_Templates;   break;
        case DlgObj::PanelNotes:            target = KeyTarget::PanelOpen_Notes;       break;
        case DlgObj::PanelModels:           target = KeyTarget::PanelOpen_Models;      break;
        case DlgObj::PanelQueue:            target = KeyTarget::PanelOpen_Queue;       break;
        default:
            LogCError("Unhandled panel " << cast::to_i32(std::get<0>(pair)));
        }

        set_hotkey_tooltip(g::pHotkeysManager->get_key_binding_name(target), std::get<2>(pair));
    }

    set_hotkey_tooltip(g::pHotkeysManager->get_key_binding_name(KeyTarget::App_Options), m_opts_hotkey);
    set_hotkey_tooltip(g::pHotkeysManager->get_key_binding_name(KeyTarget::App_TogglePanels), m_toggle_hotkey);
    set_hotkey_tooltip(g::pHotkeysManager->get_key_binding_name(KeyTarget::PanelOpen_Logs), m_logs_hotkey);
}

void ActivityBar::calc_window_sizes(bool left, bool right) noexcept
{
    const auto* main_viewport = ImGui::GetMainViewport();

    ImGui::PushFont(g::pOptsManager->ui_activity_bar_big_icons_load() ? g::pMain->get_big_icons_font() : g::pMain->get_icons_font());
    const auto bar_width = ImGui::CalcTextSize(ICON_LC_X).x + ImGui::GetStyle().ItemSpacing.x * 2;
    ImGui::PopFont();

    if (left)
    {
        m_left_rc.Min.x = main_viewport->WorkPos.x;
        m_left_rc.Min.y = main_viewport->WorkPos.y;

        m_left_rc.Max.x = m_left_rc.Min.x + bar_width;
        m_left_rc.Max.y = m_left_rc.Min.y + main_viewport->WorkSize.y;

        //main_viewport->WorkPos.x += m_bar_width;
        //main_viewport->WorkSize.x -= m_bar_width;
        m_bar_l_width = bar_width;
    }

    if (right)
    {
        m_right_rc.Min.x = main_viewport->WorkPos.x + main_viewport->WorkSize.x - bar_width;
        m_right_rc.Min.y = main_viewport->WorkPos.y;

        m_right_rc.Max.x = m_right_rc.Min.x + bar_width;
        m_right_rc.Max.y = m_right_rc.Min.y + main_viewport->WorkSize.y;

        //main_viewport->WorkSize.x -= m_bar_width;
        m_bar_r_width = bar_width;
    }
}

void ActivityBar::recalc_viewport_size(bool left, bool right) noexcept
{
    //if (left && right)
    //    return;

    //ImGuiViewport* main_viewport = ImGui::GetMainViewport();

    //ImGui::PushFont(g::pOptsManager->ui_activity_bar_big_icons_load() ? g::pMain->get_big_icons_font() : g::pMain->get_icons_font());
    //const auto bar_width = ImGui::CalcTextSize(ICON_LC_X).x + ImGui::GetStyle().ItemSpacing.x * 2;
    //ImGui::PopFont();

    if (!left)
    {
        //main_viewport->WorkPos.x -= m_bar_width;
        //main_viewport->WorkSize.x += m_bar_width;
        m_bar_l_width = 0.0f;
    }
    if (!right)
    {
        //main_viewport->WorkSize.x += m_bar_width;
        m_bar_r_width = 0.0f;
    }
}

const ImGuiDockNode* ActivityBar::get_central_node(const ImGuiDockNode* node)
{
    if (!node)
        return nullptr;

    if (node->IsCentralNode())
        return node;

    if (const auto* res = get_central_node(node->ChildNodes[0]))
        return res;

    if (const auto* res = get_central_node(node->ChildNodes[1]))
        return res;

    return nullptr;
}

void ActivityBar::show(ActivityBarMode mode)
{
    const bool show_left = (ActivityBarMode::Smart == mode || ActivityBarMode::Left == mode);
    const bool show_right = (ActivityBarMode::Smart == mode || ActivityBarMode::Right == mode);
    const bool left_include_all = show_left;
    const bool right_include_all = (ActivityBarMode::Right == mode);

    calc_window_sizes(show_left, show_right);

    const ImGuiDockNode* root_node = ImGui::DockBuilderGetNode(g::pFlow->get_main_dockspace_id());
    const ImGuiDockNode* central_node = get_central_node(root_node);

    size_t l_panels = 0, r_panels = 0;
    for (auto& [id, direction, hotkey] : m_panels)
    {
        (void)hotkey;
        if (nullptr == central_node || !g::pFlow->is_panel_active(id)) [[unlikely]]
            direction = Direction::None;
        else
        {
            direction = get_panel_direction(id, central_node->Rect());
            l_panels += (Direction::Left == direction ? 1 : 0);
            r_panels += (Direction::Right == direction ? 1 : 0);
        }
    }

    ImGui::PushStyleColor(ImGuiCol_WindowBg, m_clr_bg);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    bool l_shown = false, r_shown = false;

    if (show_left && l_panels > 0 || left_include_all)
    {
        ImGui::SetNextWindowPos(m_left_rc.Min, ImGuiCond_Always);
        ImGui::SetNextWindowSize(m_left_rc.Max, ImGuiCond_Always);

        bool opened = true;
        if (ImGui::Begin(DlgObj_SID[cast::to_szt(DlgObj::ActivityBarL)], &opened,
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus))
        {
            for (auto& [id, direction, hotkey] : m_panels)
            {
                if (Direction::Left == direction || left_include_all)
                {
                    draw_panel_buttons(id, g::pFlow->is_panel_active(id), hotkey);
                    l_shown = true;
                }
            }
        }

        if (left_include_all)
        {
            draw_additional_buttons();
            l_shown = true;
        }
        ImGui::End();
    }

    if (show_right && r_panels > 0 || right_include_all)
    {
        ImGui::SetNextWindowPos(m_right_rc.Min, ImGuiCond_Always);
        ImGui::SetNextWindowSize(m_right_rc.Max, ImGuiCond_Always);

        bool opened = true;
        if (ImGui::Begin(DlgObj_SID[cast::to_szt(DlgObj::ActivityBarR)], &opened,
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus))
        {
            for (auto& [id, direction, hotkey] : m_panels)
            {
                if (Direction::Right == direction || right_include_all)
                {
                    draw_panel_buttons(id, g::pFlow->is_panel_active(id), hotkey);
                    r_shown = true;
                }
            }
        }

        if (right_include_all)
        {
            draw_additional_buttons();
            r_shown = true;
        }
        ImGui::End();
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    recalc_viewport_size(l_shown, r_shown);
}

ActivityBar::Direction ActivityBar::get_panel_direction(DlgObj panel_obj, ImRect center_rc) noexcept
{
    const auto panel_id = DlgObj_SID[cast::to_szt(panel_obj)];
    const ImGuiWindow* win = ImGui::FindWindowByName(panel_id);
    if (nullptr == win || nullptr == win->DockNode)
        return Direction::None;

    const ImRect panel_rc = win->DockNode->Rect();

    if (panel_rc.Min.y >= center_rc.Max.y - 5.0f)
        return Direction::Bottom;

    if (panel_rc.Max.x <= center_rc.Min.x + 5.0f)
        return Direction::Left;

    if (panel_rc.Min.x >= center_rc.Max.x - 5.0f)
        return Direction::Right;

    return Direction::None;
}

void ActivityBar::show_tooltip(FontGuard* fg, const char* text, const std::string& hotkey) noexcept
{
    if (text != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
    {
        fg->pop();
        if (ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted(text);
            if (!hotkey.empty())
            {
                ImGui::SameLine();
                ImGui::TextDisabled(hotkey.c_str());
            }
            ImGui::EndTooltip();
        }
    }
}

void ActivityBar::draw_panel_buttons(DlgObj panel_obj, bool active, const std::string& hotkey)
{
    const auto icon = g::pFlow->get_panel_icon(panel_obj);

    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);

    FontGuard fg(g::pOptsManager->ui_activity_bar_big_icons_load() ? g::pMain->get_big_icons_font() : g::pMain->get_icons_font());
    ImGui::PushStyleColor(ImGuiCol_Text, 0);
    const bool pressed = ImGui::Button(icon);
    ImGui::PopStyleColor();
    show_tooltip(&fg, g::pFlow->get_panel_name(panel_obj), hotkey);

    const auto btn_pos = ImGui::GetItemRectMin();

    if (pressed)
        active = !active;

    ImU32 color;
    if (pressed || ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        color = m_clr_pressed;
    else if (ImGui::IsItemHovered())
        color = m_clr_hover;
    else if (active)
        color = m_clr_active;
    else
        color = m_clr_inactive;

    fg.push();
    ImGui::GetWindowDrawList()->AddText(btn_pos, color, icon);

    ImGui::PopStyleColor(3);
    ImGui::Spacing();

    if (pressed)
        g::pFlow->toggle_panel_visibility(panel_obj);
}

void ActivityBar::draw_additional_buttons()
{
    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);

    FontGuard fg(g::pOptsManager->ui_activity_bar_big_icons_load() ? g::pMain->get_big_icons_font() : g::pMain->get_icons_font());

    // toggle panels
    fg.push();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.x * 3);

    ImGui::PushStyleColor(ImGuiCol_Text, 0);
    bool pressed = ImGui::Button(ICON_LC_EYE);
    ImGui::PopStyleColor();
    show_tooltip(&fg, "Toggle active panels", m_toggle_hotkey);

    auto btn_pos = ImGui::GetItemRectMin();

    ImU32 color;
    if (pressed || ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        color = m_clr_pressed;
    else if (ImGui::IsItemHovered())
        color = m_clr_hover;
    else
        color = m_clr_inactive;

    fg.push();
    ImGui::GetWindowDrawList()->AddText(btn_pos, color, ICON_LC_EYE);

    if (pressed)
        g::pFlow->on_menu_panel_toggle_active();

    // settings
    const auto h = ImGui::CalcTextSize(ICON_LC_X).y;
    ImGui::SetCursorPosY(ImGui::GetMainViewport()->WorkSize.y - h - ImGui::GetStyle().ItemSpacing.x * 2);

    fg.push();
    ImGui::PushStyleColor(ImGuiCol_Text, 0);
    pressed = ImGui::Button(ICON_LC_SETTINGS);
    ImGui::PopStyleColor();
    show_tooltip(&fg, "Options", m_opts_hotkey);

    btn_pos = ImGui::GetItemRectMin();

    if (pressed || ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        color = m_clr_pressed;
    else if (ImGui::IsItemHovered())
        color = m_clr_hover;
    else
        color = m_clr_inactive;

    fg.push();
    ImGui::GetWindowDrawList()->AddText(btn_pos, color, ICON_LC_SETTINGS);

    if (pressed)
        g::pFlow->open_settings_dlg();

    // ServerLogs

    fg.push();
    ImGui::SetCursorPosY(ImGui::GetMainViewport()->WorkSize.y - h * 2 - ImGui::GetStyle().ItemSpacing.x * 4);

    ImGui::PushStyleColor(ImGuiCol_Text, 0);
    pressed = ImGui::Button(g::pFlow->get_panel_icon(DlgObj::PanelServerLogs));
    ImGui::PopStyleColor();
    show_tooltip(&fg, g::pFlow->get_panel_name(DlgObj::PanelServerLogs), m_logs_hotkey);

    btn_pos = ImGui::GetItemRectMin();

    if (pressed || ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        color = m_clr_pressed;
    else if (ImGui::IsItemHovered())
        color = m_clr_hover;
    else if (g::pFlow->is_panel_active(DlgObj::PanelServerLogs))
        color = m_clr_active;
    else
        color = m_clr_inactive;

    fg.push();
    ImGui::GetWindowDrawList()->AddText(btn_pos, color, g::pFlow->get_panel_icon(DlgObj::PanelServerLogs));

    if (pressed)
        g::pFlow->toggle_panel_visibility(DlgObj::PanelServerLogs);

    // restart
    //if (g::pOptsManager->ui_activity_bar_show_restart_load())
    //{
    //    fg.push();
    //    ImGui::SetCursorPosY(ImGui::GetMainViewport()->WorkSize.y - h * 2 - ImGui::GetStyle().ItemSpacing.x * 4);

    //    ImGui::PushStyleColor(ImGuiCol_Text, 0);
    //    pressed = ImGui::Button(ICON_LC_POWER);
    //    ImGui::PopStyleColor();
    //    tooltip("Restart " APP_COMFYUI " server");

    //    btn_rc = ImGui::GetItemRect();

    //    if (pressed || ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    //        color = m_clr_restart_pressed;
    //    else if (ImGui::IsItemHovered())
    //        color = m_clr_restart_hover;
    //    else
    //        color = m_clr_restart_inactive;

    //    fg.push();
    //    ImGui::GetWindowDrawList()->AddText(btn_rc.Min, color, ICON_LC_POWER);

    //    //if (pressed)
    //        // TODO: restart
    //}

    // cleanup
    ImGui::PopStyleColor(3);
}

} // namespace ui
