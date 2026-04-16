
#include "ui/panels/panel_base.h"
#include "ui/helpers/font_guard.h"
#include "main.h"

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
    const TtCallbacks* tt_callbacks /*= nullptr*/, const DndCallbacks* dnd_callbacks /*= nullptr*/)
{
    bool dirty = false;
    dirty_item = {};

    static const float dbl_check_w = ImGui::CalcTextSize(ICON_LC_CHECK_CHECK).x + ImGui::GetStyle().ItemSpacing.x;
    const float indent = dbl_check_w;

    const int32_t size = cast::to_i32(items.size());
    const bool clipper_needed = size > 100;

    ImGuiListClipper clipper;
    if (clipper_needed)
        clipper.Begin(size);

    while (!clipper_needed && size > 0 || clipper_needed && clipper.Step())
    {
        const int32_t start = (clipper_needed ? clipper.DisplayStart : 0);
        const int32_t end = (clipper_needed ? clipper.DisplayEnd : size);

        for (int32_t idx = start; idx < end; idx++)
        {
            auto& item = items[idx];
            bool allow_tt = true;

            if (item.is_separator)
            {
                ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), 0.0f, item.name.c_str(), item.name.size());
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
            bool dnd = false;

            if (!item.renaming)
            {
                ImGui::SetCursorPosX(0);
                ImGui::InvisibleButton("btn", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing()));

                if (dnd_callbacks != nullptr && dnd_callbacks->on_dnd_allow(idx))
                {
                    if (ImGui::BeginDragDropSource())
                    {
                        dnd = true;
                        ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
                        ImGui::SetDragDropPayload(dnd_callbacks->dnd_id, &item.dnd_data, sizeof(item.dnd_data));
                        dnd_callbacks->on_dnd_show(idx);
                        ImGui::EndDragDropSource();
                    }
                }
                if (!dnd)
                {
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
                    {
                        m_hovered_item = idx;
                        l_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                        l_released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
                        l_dbl_clicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                        r_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
                        r_released = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
                    }
                }

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
                    if (item.icon_active) ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::Starred));
                    ImGui::TextUnformatted(item.icon);
                    if (item.icon_active) ImGui::PopStyleColor();

                    if (!dnd && item.icon_tooltip != nullptr)
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

                if (!dnd && (item.continue_processing_ico_handler || mouse_on_icon && item.on_icon_handler != nullptr))
                {
                    if (item.on_icon_handler(item, l_clicked, l_dbl_clicked, l_released, r_clicked, r_released, handler_processed))
                    {
                        dirty = true;
                        dirty_item.icon_clicked = true;
                        dirty_item.lclicked = l_clicked;
                        dirty_item.lreleased = l_released;
                        dirty_item.index = idx;
                    }
                }
                if (!dnd && !dirty && !handler_processed && (item.continue_processing_item_handler || mouse_on_name && item.on_item_handler != nullptr))
                {
                    if (item.on_item_handler(item, l_clicked, l_dbl_clicked, l_released, r_clicked, r_released, handler_processed))
                    {
                        dirty = true;
                        dirty_item.item_clicked = true;
                        dirty_item.lclicked = l_clicked;
                        dirty_item.lreleased = l_released;
                        dirty_item.index = idx;
                    }
                }
                if (!dnd && !dirty && !handler_processed && allow_tt && item.on_hover_handler != nullptr)
                {
                    if (item.on_hover_handler(item, handler_processed))
                    {
                        dirty = true;
                        dirty_item.item_handled = true;
                        dirty_item.index = idx;
                    }
                }
                if (!dnd && !dirty && !handler_processed && m_hovered_item == idx)
                {
                    if (l_clicked || l_released)
                    {
                        if (!l_released && mouse_on_icon && (item.can_expand || item.icon_can_click))
                        {
                            dirty = true;
                            dirty_item.index = idx;

                            if (item.icon_can_click)
                            {
                                item.icon_active = !item.icon_active;
                                dirty_item.icon_clicked = true;
                                dirty_item.lclicked = l_clicked;
                                dirty_item.lreleased = l_released;
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
                            if (item.can_expand && !l_released)
                            {
                                item.expanded = !item.expanded;
                                dirty_item.expanded = true;
                            }
                            else if (!item.can_expand)
                            {
                                dirty_item.lclicked = l_clicked;
                                dirty_item.lreleased = l_released;
                            }
                            else
                                dirty = false;
                            dirty_item.index = idx;
                        }
                    }
                    if (!dirty && item.can_rclick && (r_clicked || r_released))
                    {
                        dirty = true;
                        dirty_item.rclicked = r_clicked;
                        dirty_item.rreleased = r_released;
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

        if (!clipper_needed)
            break;
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
        bool show_tt = true;
        if (tt_callbacks->on_pre_tt) show_tt = tt_callbacks->on_pre_tt(tt_index);
        if (show_tt && ImGui::BeginTooltip())
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

void PanelToolbar::add_toolbar_element(Element&& el, bool left_aligned)
{
    if (left_aligned)
        m_elements_l.push_back(std::move(el));
    else
        m_elements_r.push_back(std::move(el));
}

void PanelToolbar::finalize_toolbar()
{
    m_clr_hover = g::pOptsManager->color_load(AppColor::SelectedItem);
    m_clr_pressed = ImGuiEx::increase_brightness(m_clr_hover);
}

void PanelToolbar::clear_toolbar()
{
    m_elements_l.clear();
    m_elements_r.clear();
}

void PanelToolbar::draw_toolbar()
{
    const float avail_w = ImGui::GetContentRegionAvail().x;
    const auto& style = ImGui::GetStyle();
    const float regular_h = ImGui::GetTextLineHeight();
    const float regular_padding_y = style.FramePadding.y;

    FontGuard fg(m_use_big_icons ? g::pMain->get_icons_font() : nullptr);

    const float big_h = ImGui::GetTextLineHeight();
    const ImVec2 ico_size = ImGui::CalcTextSize(ICON_LC_X);

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

    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);

    float start_pos = style.ItemSpacing.x;

    ImVec2 regular_btn_size = {};
    bool pressed = false;

    for (auto it = m_elements_l.begin(); it != m_elements_l.end(); ++it)
    {
        auto& el = *it;
        if (Element::Type::Empty == el.type)
        {
            start_pos += style.ItemSpacing.x * 2;
            continue;
        }

        ImGui::SetCursorPosX(start_pos);

        if (el.on_preview) el.on_preview(el);
        if (!el.enabled) ImGui::BeginDisabled();

        if (Element::Type::Button == el.type)
        {
            fg.pop();
            ImGui::PushStyleColor(ImGuiCol_Text, 0);
            pressed = ImGui::Button(el.icon);
            ImGui::PopStyleColor();

            if (el.tooltip != nullptr && *el.tooltip != 0)
                tooltip(el.tooltip);

            if (pressed && el.toggler)
                el.toggled = !el.toggled;

            if (0 == regular_btn_size.x)
                regular_btn_size = ImGui::GetItemRect().GetSize();

            auto btn_pos = ImGui::GetItemRectMin();
            //if (-FLT_MIN == regular_el_y)
            //    regular_el_y = btn_pos.y;

            ImU32 color;
            if (pressed || el.toggled || ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                color = m_clr_pressed;
            else if (ImGui::IsItemHovered())
                color = m_clr_hover;
            else
                color = ImGui::GetColorU32(style.Colors[ImGuiCol_Text]);

            fg.push();
            const ImVec2 ico_pos = { btn_pos.x, (btn_pos.y + regular_padding_y) - (big_h - regular_h) };
            ImGui::GetWindowDrawList()->AddText(ico_pos, color, el.icon);

            start_pos += regular_btn_size.x + style.ItemSpacing.x;
        }
        else if (Element::Type::Input == el.type || Element::Type::InputHint == el.type)
        {
            fg.pop();
            ImGui::SetNextItemWidth(avail_w * el.input_w);
            if (Element::Type::Input == el.type)
                pressed = ImGuiEx::InputText(el.id, el.buffer_str, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_AutoSelectAll);
            else
                pressed = ImGuiEx::InputTextWithHint(el.id, el.hint, el.buffer_str, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_AutoSelectAll);
            pressed = pressed || ImGui::IsItemDeactivatedAfterEdit();

            if (el.tooltip != nullptr && *el.tooltip != 0)
                tooltip(el.tooltip);

            //if (-FLT_MIN == regular_el_y)
            //    regular_el_y = ImGui::GetItemRectMin().y;

            start_pos += avail_w * el.input_w + style.ItemSpacing.x;
        }
        else if (Element::Type::InputInt == el.type)
        {
            ImGui::PopStyleColor(3);

            fg.pop();
            ImGui::SetNextItemWidth(el.input_w);
            pressed = ImGuiEx::InputIntSmart(el.id, el.buffer_int, 1, el.int_min, el.int_max, "%d", "%d");
            pressed = pressed || ImGui::IsItemDeactivatedAfterEdit();

            if (el.tooltip != nullptr && *el.tooltip != 0)
                tooltip(el.tooltip);

            start_pos += avail_w * el.input_w + style.ItemSpacing.x;

            ImGui::PushStyleColor(ImGuiCol_Button, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
        }
        else if (Element::Type::Text == el.type && el.text != nullptr)
        {
            fg.pop();
            if (el.text_clr != 0) ImGui::PushStyleColor(ImGuiCol_Text, el.text_clr);
            if (el.text_bold) ImGui::PushFont(g::pMain->get_bold_font());
            ImGui::TextUnformatted(el.text);
            if (el.text_bold) ImGui::PopFont();
            if (el.text_clr != 0) ImGui::PopStyleColor();

            if (el.tooltip != nullptr && *el.tooltip != 0)
                tooltip(el.tooltip);

            start_pos += ImGui::GetItemRectSize().x + style.ItemSpacing.x;
        }

        if (pressed && el.on_click) el.on_click(el);
        if (!el.enabled) ImGui::EndDisabled();
        if (el.on_postview) el.on_postview(el);

        if (el.reset_state)
        {
            ImGui::PopStyleColor(3);
            return;
        }

        ImGui::SameLine();
    }

    start_pos = avail_w - style.ItemSpacing.x;

    for (auto it = m_elements_r.rbegin(); it != m_elements_r.rend(); ++it)
    {
        auto& el = *it;
        if (Element::Type::Empty == el.type)
        {
            start_pos -= style.ItemSpacing.x * 2;
            continue;
        }

        if (el.on_preview) el.on_preview(el);
        if (!el.enabled) ImGui::BeginDisabled();

        if (Element::Type::Button == el.type)
        {
            ImGui::SetCursorPosX(start_pos);

            fg.pop();
            ImGui::PushStyleColor(ImGuiCol_Text, 0);
            pressed = ImGui::Button(el.icon);
            ImGui::PopStyleColor();

            if (el.tooltip != nullptr && *el.tooltip != 0)
                tooltip(el.tooltip);

            if (pressed && el.toggler)
                el.toggled = !el.toggled;

            if (0 == regular_btn_size.x)
                regular_btn_size = ImGui::GetItemRect().GetSize();

            auto btn_pos = ImGui::GetItemRectMin();
            //if (-FLT_MIN == regular_el_y)
            //    regular_el_y = btn_pos.y;

            ImU32 color;
            if (pressed || el.toggled || ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                color = m_clr_pressed;
            else if (ImGui::IsItemHovered())
                color = m_clr_hover;
            else
                color = ImGui::GetColorU32(style.Colors[ImGuiCol_Text]);

            fg.push();
            const ImVec2 ico_pos = { btn_pos.x, (btn_pos.y + regular_padding_y) - (big_h - regular_h) };
            ImGui::GetWindowDrawList()->AddText(ico_pos, color, el.icon);

            start_pos -= regular_btn_size.x + style.ItemSpacing.x;
        }
        else if (Element::Type::Input == el.type || Element::Type::InputHint == el.type)
        {
            start_pos -= (avail_w * el.input_w);
            ImGui::SetCursorPosX(start_pos);

            fg.pop();
            ImGui::SetNextItemWidth(avail_w * el.input_w);
            if (Element::Type::Input == el.type)
                pressed = ImGuiEx::InputText(el.id, el.buffer_str, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_AutoSelectAll);
            else
                pressed = ImGuiEx::InputTextWithHint(el.id, el.hint, el.buffer_str, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_AutoSelectAll);
            pressed = pressed || ImGui::IsItemDeactivatedAfterEdit();

            if (el.tooltip != nullptr && *el.tooltip != 0)
                tooltip(el.tooltip);

            //if (-FLT_MIN == regular_el_y)
            //    regular_el_y = ImGui::GetItemRectMin().y;

            start_pos -= regular_btn_size.x + style.ItemSpacing.x;
        }
        else if (Element::Type::InputInt == el.type)
        {
            ImGui::PopStyleColor(3);

            start_pos -= el.input_w;
            ImGui::SetCursorPosX(start_pos);

            fg.pop();
            ImGui::SetNextItemWidth(el.input_w);
            pressed = ImGuiEx::InputIntSmart(el.id, el.buffer_int, 1, el.int_min, el.int_max, "%d", "%d");
            pressed = pressed || ImGui::IsItemDeactivatedAfterEdit();

            if (el.tooltip != nullptr && *el.tooltip != 0)
                tooltip(el.tooltip);

            start_pos -= regular_btn_size.x + style.ItemSpacing.x;

            ImGui::PushStyleColor(ImGuiCol_Button, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
        }
        else if (Element::Type::Text == el.type && el.text != nullptr)
        {
            fg.pop();
            auto text_w = ImGui::CalcTextSize(el.text).x;
            start_pos -= text_w - style.ItemSpacing.x;
            ImGui::SetCursorPosX(start_pos);

            if (el.text_clr != 0) ImGui::PushStyleColor(ImGuiCol_Text, el.text_clr);
            if (el.text_bold) ImGui::PushFont(g::pMain->get_bold_font());
            ImGui::TextUnformatted(el.text);
            if (el.text_bold) ImGui::PopFont();
            if (el.text_clr != 0) ImGui::PopStyleColor();

            if (el.tooltip != nullptr && *el.tooltip != 0)
                tooltip(el.tooltip);

            start_pos -= regular_btn_size.x + style.ItemSpacing.x;
        }

        if (pressed && el.on_click) el.on_click(el);
        if (!el.enabled) ImGui::EndDisabled();
        if (el.on_postview) el.on_postview(el);

        if (el.reset_state)
            break;

        ImGui::SameLine();
    }

    ImGui::PopStyleColor(3);
    ImGui::Spacing();
}

} // namespace ui
