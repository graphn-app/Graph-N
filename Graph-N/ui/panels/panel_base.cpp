/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

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
    LogRAssert(header_descr, "Zero ptr", );
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
    float start_y = (avail.y - total_h) * 0.3f;
    if (start_y < 20.0f)
        start_y = 20.0f; // Safety margin

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + start_y);

    // Helper lambda for centered text
    const auto text_centered = [&](const char *text, bool disabled, bool as_url) noexcept
    {
        bool click = false;
        const float w = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((avail.x - w) * 0.5f);
        if (disabled)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        if (as_url)
            click = ImGuiEx::UrlButton(url);
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

    if (!titles.empty())
        ImGui::Dummy(ImVec2(0, spacing * 2)); // Extra gap

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

bool PanelWithList::draw_list(const std::vector<PanelListItem>& items, PanelItemEvent& out_event,
    const TtCallbacks* tt_callbacks, const DndCallbacks* dnd_callbacks)
{
    out_event = {};
    bool is_any_hovered = false;
    bool dirty = false;

    static const float indent = ImGui::CalcTextSize(ICON_LC_CHECK_CHECK).x + ImGui::GetStyle().ItemSpacing.x;
    const int32_t size = cast::to_i32(items.size());
    const bool clipper_needed = size > 100;

    ImGuiListClipper clipper;
    if (clipper_needed)
        clipper.Begin(size);

    while ((!clipper_needed && size > 0) || (clipper_needed && clipper.Step()))
    {
        const int32_t start = (clipper_needed ? clipper.DisplayStart : 0);
        const int32_t end = (clipper_needed ? clipper.DisplayEnd : size);

        for (int32_t idx = start; idx < end; idx++)
        {
            const auto& item = items[idx];
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

            ImGui::PushID(idx);
            ImGui::BeginGroup();

            bool dnd = false;

            if (m_renaming_idx != idx) 
            {
                ImGui::SetCursorPosX(0);
                ImGui::InvisibleButton("btn", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing()));

                if (dnd_callbacks != nullptr && dnd_callbacks->on_dnd_allow(idx))
                {
                    if (ImGui::BeginDragDropSource())
                    {
                        dnd = true;
                        ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
                        ImGui::SetDragDropPayload(dnd_callbacks->dnd_id, &item.ext_data.dnd_data, sizeof(item.ext_data.dnd_data));
                        dnd_callbacks->on_dnd_show(idx);
                        ImGui::EndDragDropSource();
                    }
                }

                if (!dnd && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
                {
                    m_hovered_item = idx;
                    is_any_hovered = true;

                    const bool l_clk = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                    const bool l_rel = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
                    const bool r_clk = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
                    const bool r_rel = ImGui::IsMouseReleased(ImGuiMouseButton_Right);

                    if (l_clk || l_rel || r_clk || r_rel)
                    {
                        dirty = true;
                        out_event.index = idx;
                        out_event.l_clicked = l_clk;
                        out_event.l_released = l_rel;
                        out_event.r_clicked = r_clk;
                        out_event.r_released = r_rel;
                    }
                }
                ImGui::SameLine();
            }

            ImGui::SetCursorPosX(indent * item.depth);
            const bool is_selected = item.selected || m_hovered_item == idx;

            if (item.can_expand)
            {
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

                if (item.icon != nullptr && ((item.icon_can_click && idx == m_hovered_item) || item.icon_always))
                {
                    if (item.icon_always) ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::Starred));
                    ImGui::TextUnformatted(item.icon);
                    if (item.icon_always) ImGui::PopStyleColor();

                    if (!dnd && item.icon_tooltip != nullptr)
                    {
                        if (is_selected) ImGui::PopStyleColor();
                        ImGuiEx::TooltipOnLastControl(item.icon_tooltip, nullptr);
                        if (is_selected) ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));
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
            ico_rect.Min.x--; ico_rect.Min.y -= 2;
            ico_rect.Max.x++; ico_rect.Max.y += 3;

            ImRect name_rect = ImGui::GetItemRect(); 

            ImGui::SameLine();

            if (m_renaming_idx == idx) 
            {
                allow_tt = false;
                if (m_need_rename_focus)
                {
                    ImGui::SetKeyboardFocusHere();
                    m_need_rename_focus = false;
                }

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                const bool enter_pressed = ImGuiEx::InputText("##newname", &m_rename_buf, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    m_renaming_idx = -1;
                }
                else if (enter_pressed || ImGui::IsItemDeactivated() || 
                    ((ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) && !ImGui::IsItemHovered()))
                {
                    if (!m_rename_buf.empty() && m_rename_buf != item.name)
                    {
                        dirty = true;
                        out_event.index = idx;
                        out_event.rename_completed = true;
                        out_event.new_name = m_rename_buf;
                    }
                    m_renaming_idx = -1;
                }
            }
            else
            {
                if (is_selected) ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));

                if (item.can_expand) ImGui::PushFont(g::pMain->get_bold_font());
                ImGui::TextUnformatted(item.name.c_str(), item.name.c_str() + item.name.size());
                if (item.can_expand) ImGui::PopFont();

                if (item.can_expand && !item.name_label.empty())
                {
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    ImGui::PushFont(g::pMain->get_small_font());
                    ImGui::TextUnformatted(item.name_label.c_str(), item.name_label.c_str() + item.name_label.size());
                    ImGui::PopFont();
                    ImGui::EndDisabled();
                }

                if (is_selected) ImGui::PopStyleColor();

                name_rect = ImGui::GetItemRect();
                name_rect.Min.y -= 2;
                name_rect.Max.x = ImGui::GetWindowPos().x + ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x;
                name_rect.Max.y += 3;

                if (!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                    allow_tt = false;

                if (allow_tt && !dnd)
                    go_tooltip(idx, tt_callbacks);
            }

            if (dirty && out_event.index == idx && !dnd)
            {
                const ImVec2 mouse = ImGui::GetMousePos();

                out_event.icon_clicked = ico_rect.Contains(mouse);
                out_event.item_clicked = name_rect.Contains(mouse);

                if (item.can_expand && out_event.l_clicked)
                {
                    out_event.expand_toggled = true;
                }
            }

            ImGui::EndGroup();
            ImGui::PopID();
        }

        if (!clipper_needed)
            break;
    }

    if (!is_any_hovered)
        m_hovered_item = -1;

    if (release_tooltip(m_hovered_item, tt_callbacks))
        m_hovered_item = -1;        

    return dirty;
}

void PanelWithList::begin_rename(int32_t index, const std::string& current_name)
{
    m_renaming_idx = index;
    m_rename_buf = current_name;
    m_need_rename_focus = true;
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

void PanelToolbar::finalize_toolbar(bool add_separator) noexcept
{
    m_clr_hover = g::pOptsManager->color_load(AppColor::SelectedItem);
    m_clr_pressed = ImGuiEx::increase_brightness(m_clr_hover);
    m_draw_separator = add_separator;
}

void PanelToolbar::clear_toolbar() noexcept
{
    m_elements_l.clear();
    m_elements_r.clear();
}

void PanelToolbar::draw_background()
{
    const ImVec4 bg_color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];

    // Define lighten factor (1.0f = original, 1.05f = +5%, 1.10f = +10%)
    constexpr float lighten_factor = 1.5f; 

    // Apply factor, clamp to 1.0f to avoid overflow, and convert to 0-255 range
    const int r = static_cast<int>(std::fminf(bg_color.x * lighten_factor, 1.0f) * 255.0f);
    const int g = static_cast<int>(std::fminf(bg_color.y * lighten_factor, 1.0f) * 255.0f);
    const int b = static_cast<int>(std::fminf(bg_color.z * lighten_factor, 1.0f) * 255.0f);
    const int a = static_cast<int>(bg_color.w * 255.0f);

    // Create final color in IM_COL32(RGBA) format
    const ImU32 color = IM_COL32(r, g, b, a);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 1. ImDrawList requires absolute screen coordinates
    ImVec2 p_min = ImGui::GetCursorScreenPos();

    // 2. Define dimensions. Using GetFrameHeight() to match standard button heights
    const float width = ImGui::GetContentRegionAvail().x;
    const float height = ImGui::GetFrameHeight(); 

    ImVec2 p_max = { p_min.x + width, p_min.y + height };

    // 3. Background color in IM_COL32(RGBA) format
    //const ImU32 color = IM_COL32(128, 128, 128, 128); // Semi-transparent gray

    // 4. Rounding. Setting it to height / 2 creates a perfect capsule (pill) shape
    const float rounding = height * 0.5f;

    // 5. Draw the rounded rectangle
    draw_list->AddRectFilled(p_min, p_max, color, rounding);
}

void PanelToolbar::draw_toolbar()
{
    FontGuard fg(m_use_big_icons ? g::pMain->get_icons_font() : g::pMain->get_regular_font());

    const float avail_w = ImGui::GetContentRegionAvail().x;
    const auto& style = ImGui::GetStyle();
    const float regular_h = g::pMain->get_regular_font()->FontSize;
    const float big_h = m_use_big_icons ? g::pMain->get_icons_font()->FontSize : regular_h;
    const float regular_padding_y = style.FramePadding.y;

    constexpr double blinking_interval = 0.65;
    const bool blinking_item_show = fmod(ImGui::GetTime(), blinking_interval * 2) < blinking_interval;

    const auto tooltip = [&fg](const char* text, const std::string& hotkey, bool tooltip_next_line, const char* separator)
    {
        if (text != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        {
            if (ImGui::BeginTooltip())
            {
                fg.pop();
                ImGui::TextUnformatted(text);
                if (!hotkey.empty())
                {
                    if (!tooltip_next_line)
                        ImGui::SameLine();
                    if (separator != nullptr && *separator != 0)
                        ImGui::TextDisabled("%s%s", separator, hotkey.c_str());
                    else
                        ImGui::TextDisabled(hotkey.c_str());
                }
                ImGui::EndTooltip();
            }
        }
    };

    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);

    float start_pos = style.ItemSpacing.x;
    bool pressed = false;

    m_reset_state = false;
    const auto test_reset = [this]()
    {
        const bool reset = m_reset_state;
        m_reset_state = false;
        if (reset)
            ImGui::PopStyleColor(3);
        return reset;
    };

    const auto get_actual_text_size = [&fg](Element& el) -> float
    {
        if (el.text_width > 0.0f)
        {
            ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), el.text_width, -1.0f, el.text, nullptr, &el.text_end);
            return el.text_width;
        }
        el.text_end = el.text + strlen(el.text);
        return ImGui::CalcTextSize(el.text, el.text_end).x;
    };

    const auto calc_item_layout_w = [&fg, &style, avail_w, &get_actual_text_size](Element& el) -> float
    {
        if (Element::Type::Button == el.type)
        {
            fg.pop(); 
            const float w = ImGui::CalcTextSize(el.icon).x + style.FramePadding.x * 2.0f;
            return w;
        }
        if (Element::Type::Input == el.type || Element::Type::InputHint == el.type) return avail_w * el.input_w;
        if (Element::Type::InputInt == el.type || Element::Type::Combo == el.type) return el.input_w; // Строго в пикселях
        if (Element::Type::Text == el.type && el.text != nullptr)
        {
            if (el.text_bold) ImGui::PushFont(g::pMain->get_bold_font());
            const float w = get_actual_text_size(el);
            if (el.text_bold) ImGui::PopFont();
            return w;
        }
        return 0.0f;
    };

    // --- LEFT ELEMENTS ---

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
        if (test_reset()) return;

        ImGui::PushID(&el);

        const bool enabled = el.enabled;
        if (!enabled) ImGui::BeginDisabled();

        float item_w = calc_item_layout_w(el);

        if (Element::Type::Button == el.type)
        {
            fg.pop();
            ImGui::PushStyleColor(ImGuiCol_Text, 0);
            pressed = ImGui::Button(el.icon);
            item_w = ImGui::GetItemRectSize().x;
            ImGui::PopStyleColor();

            if (el.tooltip != nullptr && *el.tooltip != 0) tooltip(el.tooltip, el.tooltip_hotkey, el.tooltip_next_line, el.tooltip_hotkey_separator);

            if (pressed && el.toggler) el.toggled = !el.toggled;

            if (!el.blinking || (el.blinking && blinking_item_show))
            {
                const auto btn_pos = ImGui::GetItemRectMin();
                ImU32 color;
                if (pressed || el.toggled || (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))) color = m_clr_pressed;
                else if (ImGui::IsItemHovered()) color = m_clr_hover;
                else color = ImGui::GetColorU32(style.Colors[ImGuiCol_Text]);

                fg.push();
                const ImVec2 ico_pos = { btn_pos.x, (btn_pos.y + regular_padding_y) - (big_h - regular_h) };
                ImGui::GetWindowDrawList()->AddText(ico_pos, color, el.icon);
            }
        }
        else if (Element::Type::Input == el.type || Element::Type::InputHint == el.type)
        {
            fg.pop();
            ImGui::SetNextItemWidth(item_w);
            if (Element::Type::Input == el.type)
                pressed = ImGuiEx::InputText(el.id, el.buffer_str, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_AutoSelectAll);
            else
                pressed = ImGuiEx::InputTextWithHint(el.id, el.hint, el.buffer_str, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_AutoSelectAll);
            pressed = pressed || ImGui::IsItemDeactivatedAfterEdit();

            if (el.tooltip != nullptr && *el.tooltip != 0) tooltip(el.tooltip, el.tooltip_hotkey, el.tooltip_next_line, el.tooltip_hotkey_separator);
        }
        else if (Element::Type::InputInt == el.type)
        {
            fg.pop();
            ImGui::PopStyleColor(3);
            ImGui::SetNextItemWidth(item_w);
            pressed = ImGuiEx::InputIntSmart(el.id, el.buffer_int, 1, el.int_min, el.int_max, "%d", "%d")
                || ImGui::IsItemDeactivatedAfterEdit();

            if (el.tooltip != nullptr && *el.tooltip != 0) tooltip(el.tooltip, el.tooltip_hotkey, el.tooltip_next_line, el.tooltip_hotkey_separator);

            ImGui::PushStyleColor(ImGuiCol_Button, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
        }
        else if (Element::Type::Text == el.type && el.text != nullptr)
        {
            fg.pop();
            if (el.text_bold) ImGui::PushFont(g::pMain->get_bold_font());

            if (el.blinking && !blinking_item_show) ImGui::PushStyleColor(ImGuiCol_Text, 0);
            else if (el.text_clr != 0) ImGui::PushStyleColor(ImGuiCol_Text, el.text_clr);

            ImGui::TextUnformatted(el.text, el.text_end);

            if (el.text_bold) ImGui::PopFont();
            if ((el.blinking && !blinking_item_show) || el.text_clr != 0) ImGui::PopStyleColor();

            if (el.tooltip != nullptr && *el.tooltip != 0) tooltip(el.tooltip, el.tooltip_hotkey, el.tooltip_next_line, el.tooltip_hotkey_separator);
        }
        else if (Element::Type::Combo == el.type)
        {
            fg.pop();
            ImGui::PopStyleColor(3);
            ImGui::SetNextItemWidth(item_w);
            pressed = ImGui::Combo(el.id, &el.cb_item_sel, el.cb_items.data(), cast::to_i32(el.cb_items.size()))
                || ImGui::IsItemDeactivatedAfterEdit();

            if (el.tooltip != nullptr && *el.tooltip != 0) tooltip(el.tooltip, el.tooltip_hotkey, el.tooltip_next_line, el.tooltip_hotkey_separator);

            ImGui::PushStyleColor(ImGuiCol_Button, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
        }

        start_pos += item_w + style.ItemSpacing.x;

        if (pressed && el.on_click) el.on_click(el);
        if (!enabled) ImGui::EndDisabled();
        ImGui::PopID();

        if (test_reset()) return;
        if (el.on_postview) el.on_postview(el);
        if (test_reset()) return;

        ImGui::SameLine(0, 0);
    }

    // --- RIGHT ELEMENTS ---

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
        if (test_reset()) return;

        ImGui::PushID(&el);

        float item_w = calc_item_layout_w(el);
        start_pos -= item_w;
        ImGui::SetCursorPosX(start_pos);

        const bool enabled = el.enabled;
        if (!enabled) ImGui::BeginDisabled();
        pressed = false;

        if (Element::Type::Button == el.type)
        {
            fg.pop();
            ImGui::PushStyleColor(ImGuiCol_Text, 0);
            pressed = ImGui::Button(el.icon);
            ImGui::PopStyleColor();

            if (el.tooltip != nullptr && *el.tooltip != 0)
                tooltip(el.tooltip, el.tooltip_hotkey, el.tooltip_next_line, el.tooltip_hotkey_separator);

            if (pressed && el.toggler) el.toggled = !el.toggled;

            if (!el.blinking || (el.blinking && blinking_item_show))
            {
                const auto btn_pos = ImGui::GetItemRectMin();
                ImU32 color;
                if (pressed || el.toggled || (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))) color = m_clr_pressed;
                else if (ImGui::IsItemHovered()) color = m_clr_hover;
                else color = ImGui::GetColorU32(style.Colors[ImGuiCol_Text]);

                fg.push();
                const ImVec2 ico_pos = { btn_pos.x, (btn_pos.y + regular_padding_y) - (big_h - regular_h) };
                ImGui::GetWindowDrawList()->AddText(ico_pos, color, el.icon);
            }
        }
        else if (Element::Type::Input == el.type || Element::Type::InputHint == el.type)
        {
            fg.pop();
            ImGui::SetNextItemWidth(item_w);
            if (Element::Type::Input == el.type)
                pressed = ImGuiEx::InputText(el.id, el.buffer_str, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_AutoSelectAll);
            else
                pressed = ImGuiEx::InputTextWithHint(el.id, el.hint, el.buffer_str, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_AutoSelectAll);
            pressed |= ImGui::IsItemDeactivatedAfterEdit();

            if (el.tooltip != nullptr && *el.tooltip != 0) tooltip(el.tooltip, el.tooltip_hotkey, el.tooltip_next_line, el.tooltip_hotkey_separator);
        }
        else if (Element::Type::InputInt == el.type)
        {
            fg.pop();
            ImGui::PopStyleColor(3);
            ImGui::SetNextItemWidth(item_w);
            pressed = ImGuiEx::InputIntSmart(el.id, el.buffer_int, 1, el.int_min, el.int_max, "%d", "%d")
                || ImGui::IsItemDeactivatedAfterEdit();

            if (el.tooltip != nullptr && *el.tooltip != 0) tooltip(el.tooltip, el.tooltip_hotkey, el.tooltip_next_line, el.tooltip_hotkey_separator);

            ImGui::PushStyleColor(ImGuiCol_Button, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
        }
        else if (Element::Type::Text == el.type && el.text != nullptr)
        {
            fg.pop();
            if (el.text_bold) ImGui::PushFont(g::pMain->get_bold_font());

            if (el.blinking && !blinking_item_show) ImGui::PushStyleColor(ImGuiCol_Text, 0);
            else if (el.text_clr != 0) ImGui::PushStyleColor(ImGuiCol_Text, el.text_clr);

            ImGui::TextUnformatted(el.text, el.text_end);

            if ((el.blinking && !blinking_item_show) || el.text_clr != 0) ImGui::PopStyleColor();
            if (el.text_bold) ImGui::PopFont();

            if (el.tooltip != nullptr && *el.tooltip != 0) tooltip(el.tooltip, el.tooltip_hotkey, el.tooltip_next_line, el.tooltip_hotkey_separator);
        }
        else if (Element::Type::Combo == el.type)
        {
            fg.pop();
            ImGui::PopStyleColor(3);
            ImGui::SetNextItemWidth(item_w);
            pressed = ImGui::Combo(el.id, &el.cb_item_sel, el.cb_items.data(), cast::to_i32(el.cb_items.size()))
                || ImGui::IsItemDeactivatedAfterEdit();

            if (el.tooltip != nullptr && *el.tooltip != 0) tooltip(el.tooltip, el.tooltip_hotkey, el.tooltip_next_line, el.tooltip_hotkey_separator);

            ImGui::PushStyleColor(ImGuiCol_Button, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0);
        }

        if (pressed && el.on_click) el.on_click(el);
        if (!enabled) ImGui::EndDisabled();
        ImGui::PopID();

        if (test_reset()) return;
        if (el.on_postview) el.on_postview(el);
        if (test_reset()) return;

        start_pos -= style.ItemSpacing.x;

        ImGui::SameLine(0, 0);
    }

    ImGui::PopStyleColor(3);

    ImGui::NewLine();

    if (m_draw_separator)
        ImGui::Separator();
}

} // namespace ui
