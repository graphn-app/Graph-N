
#include "ui/panels/panel_base.h"
#include "ui/helpers/font_guard.h"
#include "platform/main.h"

namespace ui
{

///////////////////////////////////////////////////////////////////////
//////////////      PanelEmptyWithDescr
///////////////////////////////////////////////////////////////////////

void PanelEmptyWithDescr::show_empty_panel_descr(
    const char* header_descr,
    const std::vector<const char*>& titles,
    const std::vector<const char*>& subtitles,
    const char* url /*= nullptr*/, std::function<void()> url_handler /*= nullptr*/)
{
    ImGui::Spacing();

    // --- Static Description (Top of the window) ---
    // Make it slightly dimmed to act as a "subtitle"
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextWrapped(header_descr);
    ImGui::PopStyleColor();

    if (*header_descr)
        ImGui::Separator(); // Visually separate help from state

    // --- Center the "Empty State" in the remaining space ---

    // Calculate total height to center vertically
    const float line_h = ImGui::GetTextLineHeight();
    const float spacing = ImGui::GetStyle().ItemSpacing.y;
    // Icon + Title + Spacing + Subtitle + Help1 + Help2
    const float total_h = line_h * 7 + spacing * 4;

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    float start_y = (avail.y - total_h) * 0.5f;
    if (start_y < 20.0f)
        start_y = 20.0f; // Safety margin

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + start_y);

    // Helper lambda for centered text
    const auto text_centered = [&](const char* text, bool disabled, bool as_url)
    {
        bool click = false;
        const float w = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((avail.x - w) * 0.5f);
        if (disabled)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        if (as_url)
            click = ImGuiEx::UrlButton(url, g::pOptsManager->color_load(AppColor::UrlTextDefault), g::pOptsManager->color_load(AppColor::UrlTextHovered), nullptr);
        else
            ImGui::TextUnformatted(text);
        if (disabled)
            ImGui::PopStyleColor();
        return click;
    };

    // Draw Content
    for (auto& title : titles)
    {
        text_centered(title, false, false);
    }

    ImGui::Dummy(ImVec2(0, spacing * 2)); // Extra gap

                                          // Subtitles (Grey)
    for (auto& subtitle : subtitles)
    {
        text_centered(subtitle, true, false);
    }

    if (url != nullptr && url_handler != nullptr)
    {
        ImGui::Spacing();
        if (text_centered(url, false, true))
            url_handler();
    }
}


///////////////////////////////////////////////////////////////////////
//////////////      PanelWithList
///////////////////////////////////////////////////////////////////////

bool PanelWithList::draw_list(
    std::vector<PanelListItem>& items, PanelItemState& dirty_item,
    const TtCallbacks* tt_callbacks /*= nullptr*/)
{
    bool dirty = false;
    dirty_item = {};

    static const float dbl_check_w = ImGui::CalcTextSize(ICON_LC_CHECK_CHECK).x + ImGui::GetStyle().ItemSpacing.x;
    const float indent = dbl_check_w;

    ImGuiListClipper clipper;
    clipper.Begin(cast::to_i32(items.size()));

    while (clipper.Step())
    {
        for (int32_t idx = clipper.DisplayStart; idx < clipper.DisplayEnd; idx++)
        {
            auto& item = items[idx];
            bool allow_tt = true;

            if (item.is_separator)
            {
                ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), item.name.c_str());
                continue;
            }
            if (item.is_empty)
            {
                ImGui::TextUnformatted("");
                continue;
            }

            ImGui::PushID(&item);
            ImGui::BeginGroup();

            bool l_clicked = false;
            bool l_dbl_clicked = false;
            bool l_released = false;            
            bool r_clicked = false;
            bool r_released = false;

            if (!item.renaming)
            {
                ImGui::SetCursorPosX(0);
                ImGui::InvisibleButton("btn", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing()));

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
                    m_hovered_item = idx;

                l_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                l_dbl_clicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                l_released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
                r_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
                r_released = ImGui::IsMouseReleased(ImGuiMouseButton_Right);

                ImGui::SameLine();
            }
            ImGui::SetCursorPosX(indent * item.depth);

            bool is_selected = item.selected || m_hovered_item == idx;

            if (item.can_expand)
            {
                is_selected = false;
                if (item.icon != nullptr)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(m_hovered_item == idx ? AppColor::SelectedItem : AppColor::FolderIcon));
                    ImGui::TextUnformatted(item.icon);
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                }
            }
            else
            {
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));

                if (item.icon != nullptr && (item.icon_can_click && idx == m_hovered_item || item.icon_active))
                {
                    ImGui::TextUnformatted(item.icon);
                    if (item.icon_tooltip != nullptr)
                    {
                        if (is_selected)
                            ImGui::PopStyleColor();
                        ImGuiEx::TooltipOnLastControl(item.icon_tooltip, nullptr);
                        if (is_selected)
                            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));
                    }
                }
                else
                {
                    ImGui::TextUnformatted(ICON_LC_DOT);
                }

                if (is_selected)
                    ImGui::PopStyleColor();
            }

            ImRect ico_rect = ImGui::GetItemRect();
            ico_rect.Min.x--;
            ico_rect.Min.y -= 2;
            ico_rect.Max.x++;
            ico_rect.Max.y += 3;

            ImGui::SameLine();

            if (item.renaming)
            {
                allow_tt = false;

                if (item.need_focus)
                    ImGui::SetKeyboardFocusHere();
                item.need_focus = false;

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                const bool enter_pressed = ImGuiEx::InputText("##newname", &item.name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    item.name = item.name_backup;
                    item.name_backup = {};
                    item.renaming = false;
                }

                if (enter_pressed || ImGui::IsItemDeactivated() ||
                    ((ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) && !ImGui::IsItemHovered()))
                {
                    if (item.name != item.name_backup)
                    {
                        if (item.name.empty())
                        {
                            item.name = item.name_backup;
                            item.name_backup = {};
                        }
                        else
                        {
                            dirty = true;
                            dirty_item.renamed = true;
                            dirty_item.index = idx;
                        }
                    }
                    item.renaming = false;
                }
            }
            else
            {
                if (is_selected || m_hovered_item == idx)
                    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));
                if (item.can_expand)
                    ImGui::PushFont(g::pMain->get_bold_font());
                ImGui::TextUnformatted(item.name.c_str(), item.name.c_str() + item.name.size());
                if (item.can_expand)
                {
                    ImGui::PopFont();
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    ImGui::PushFont(g::pMain->get_small_font());
                    ImGui::TextUnformatted(item.name_p2.c_str(), item.name_p2.c_str() + item.name_p2.size());
                    ImGui::PopFont();
                    ImGui::EndDisabled();
                }
                if (is_selected || m_hovered_item == idx)
                    ImGui::PopStyleColor();

                if (!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                    allow_tt = false;

                ImRect name_rect = ImGui::GetItemRect();
                name_rect.Min.y -= 2;
                name_rect.Max.x = ImGui::GetWindowPos().x + ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x;
                name_rect.Max.y += 3;

                const auto mouse = ImGui::GetMousePos();
                const bool mouse_on_icon = ico_rect.Contains(mouse);
                const bool mouse_on_name = name_rect.Contains(mouse);
                bool handler_processed = false;

                if (item.continue_processing_ico_handler || mouse_on_icon && item.on_icon_handler != nullptr)
                {
                    if (item.on_icon_handler(item, l_clicked, l_dbl_clicked, l_released, r_clicked, r_released, handler_processed))
                    {
                        dirty = true;
                        dirty_item.icon_clicked = true;
                        dirty_item.index = idx;
                    }
                }
                if (!dirty && !handler_processed && (item.continue_processing_item_handler || mouse_on_name && item.on_item_handler != nullptr))
                {
                    if (item.on_item_handler(item, l_clicked, l_dbl_clicked, l_released, r_clicked, r_released, handler_processed))
                    {
                        dirty = true;
                        dirty_item.item_clicked = true;
                        dirty_item.index = idx;
                    }
                }
                if (!dirty && !handler_processed && allow_tt && item.on_hover_handler != nullptr)
                {
                    if (item.on_hover_handler(item, handler_processed))
                    {
                        dirty = true;
                        dirty_item.item_handled = true;
                        dirty_item.index = idx;
                    }
                }
                if (!dirty && !handler_processed && m_hovered_item == idx)
                {
                    if (l_clicked)
                    {
                        if (mouse_on_icon && (item.can_expand || item.icon_can_click))
                        {
                            dirty = true;
                            dirty_item.index = idx;

                            if (item.icon_can_click)
                            {
                                item.icon_active = !item.icon_active;
                                dirty_item.icon_clicked = true;
                            }
                            else // if (item.can_expand)
                            {
                                item.expanded = !item.expanded;
                                dirty_item.expanded = true;
                            }
                        }

                        if (!dirty && (item.can_lclick || item.can_expand))
                        {
                            dirty = true;
                            if (item.can_expand)
                            {
                                item.expanded = !item.expanded;
                                dirty_item.expanded = true;
                            }
                            else
                            {
                                dirty_item.lclicked = true;
                            }
                            dirty_item.index = idx;
                        }
                    }
                    if (!dirty && item.can_rclick && r_clicked)
                    {
                        dirty = true;
                        dirty_item.rclicked = true;
                        dirty_item.index = idx;
                    }
                    if (!dirty && mouse_on_name && item.can_dblclick && l_dbl_clicked)
                    {
                        dirty = true;
                        dirty_item.dblclicked = true;
                        dirty_item.index = idx;
                    }
                    if (allow_tt)
                    {
                        go_tooltip(idx, tt_callbacks);
                    }
                }
            }

            ImGui::EndGroup();
            ImGui::PopID();
        }
    }

    if (release_tooltip(m_hovered_item, tt_callbacks))
        m_hovered_item = -1;        

    return dirty;
}

void PanelWithList::go_tooltip(int32_t tt_index, const TtCallbacks* tt_callbacks)
{
    m_tt_life_timer = (tt_callbacks && tt_callbacks->on_alloc_tt_data) ? 0.5f : 0.1f;

    if (-1 == m_tt_index || tt_index != m_tt_index)
    {
        m_tt_index = tt_index;
        if (tt_callbacks && tt_callbacks->on_alloc_tt_data) tt_callbacks->on_alloc_tt_data(m_tt_index);
    }

    if (tt_callbacks && tt_callbacks->on_show_tt)
    {
        if (tt_callbacks->on_pre_tt) tt_callbacks->on_pre_tt(tt_index);
        if (ImGui::BeginTooltip())
        {
            tt_callbacks->on_show_tt(tt_index);
            ImGui::EndTooltip();
        }
        if (tt_callbacks->on_post_tt) tt_callbacks->on_post_tt(tt_index);
    }
}

bool PanelWithList::release_tooltip(int32_t tt_index, const TtCallbacks* tt_callbacks)
{
    if (m_tt_life_timer > 0)
    {
        m_tt_life_timer -= ImGui::GetIO().DeltaTime;
        return false;
    }

    m_tt_index = -1;
    if (tt_callbacks && tt_callbacks->on_release_tt_data) tt_callbacks->on_release_tt_data(tt_index);
    return true;
}

///////////////////////////////////////////////////////////////////////
//////////////      PanelToolbar
///////////////////////////////////////////////////////////////////////

void PanelToolbar::add_button(Button&& button)
{
    //FontGuard fg(g::pMain->get_icons_font());
    //if (0.0f == button.ico_width)
    //    button.ico_width = ImGui::CalcTextSize(button.icon).x;
    m_buttons.push_back(std::move(button));
}

void PanelToolbar::finalize_toolbar()
{
    m_clr_hover = g::pOptsManager->color_load(AppColor::SelectedItem);

    ImVec4 c = ImGui::ColorConvertU32ToFloat4(m_clr_hover);
    c.x = std::min(c.x * 0.8f, 1.0f); // R
    c.y = std::min(c.y * 0.8f, 1.0f); // G
    c.z = std::min(c.z * 0.8f, 1.0f); // B
    m_clr_pressed = ImGui::ColorConvertFloat4ToU32(c);

    //const auto& style = ImGui::GetStyle();
    //m_toolbar_w = 0.0f;
    //for (auto& btn : m_buttons)
    //{
    //    m_toolbar_w += btn.ico_width + style.ItemSpacing.x * (btn.long_indent_after ? 3 : 1);
    //}
}

void PanelToolbar::clear_toolbar()
{
    m_buttons.clear();
}

void PanelToolbar::draw_toolbar()
{
    FontGuard fg(g::pMain->get_icons_font());
    const auto tooltip = [&fg](const char* text)
    {
        if (text != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        {
            fg.pop();
            if (ImGui::BeginTooltip())
            {
                ImGui::TextUnformatted(text);
                ImGui::EndTooltip();
            }
        }
    };

    const auto& style = ImGui::GetStyle();

    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);

    float start_pos = ImGui::GetContentRegionAvail().x - 40;
    ImVec2 btn_size = {};

    for (auto it = m_buttons.rbegin(); it != m_buttons.rend(); ++it)
    {
        auto button = *it;

        if (button.empty_indent)
        {
            start_pos -= style.ItemSpacing.x * 2;
            continue;
        }

        ImGui::SetCursorPosX(start_pos);

        if (button.on_preview) button.on_preview(button);
        if (!button.enabled) ImGui::BeginDisabled();

        ImGui::PushStyleColor(ImGuiCol_Text, 0);
        const bool pressed = ImGui::Button(button.icon);
        btn_size = ImGui::GetItemRectSize();
        const auto btn_pos = ImGui::GetItemRectMin();
        ImGui::PopStyleColor();
        tooltip(button.tooltip);

        ImU32 color; 
        if (ImGui::IsItemActive())
            color = m_clr_pressed;
        else if (ImGui::IsItemHovered())
            color = m_clr_hover;
        else
            color = ImGui::GetColorU32(style.Colors[ImGuiCol_Text]);

        fg.push();
        ImGui::GetWindowDrawList()->AddText(btn_pos, color, button.icon);

        if (pressed && button.on_click) button.on_click(button);
        if (!button.enabled) ImGui::EndDisabled();
        if (button.on_postview) button.on_postview(button);

        if (button.reset_state)
            break;

        start_pos -= btn_size.x + style.ItemSpacing.x;
        ImGui::SameLine();
    }

    ImGui::PopStyleColor(3);
}

} // namespace ui
