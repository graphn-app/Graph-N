
#include "ui/panels/panel_base.h"

namespace ui
{

bool PanelWithList::draw_list(
    std::vector<PanelListItem>& items, PanelItemState& dirty_item,
    const TtCallbacks* tt_callbacks /*= nullptr*/)
{
    bool dirty = false;
    dirty_item = {};

    static const float dbl_check_w = ImGui::CalcTextSize(ICON_LC_CHECK_CHECK).x + ImGui::GetStyle().ItemSpacing.x;
    static const float folder_w = ImGui::CalcTextSize(ICON_LC_FOLDER_OPEN).x + ImGui::GetStyle().ItemSpacing.x;
    const float indent = folder_w;

    ImGuiListClipper clipper;
    clipper.Begin(cast::to_i32(items.size()));

    while (clipper.Step())
    {
        for (int32_t idx = clipper.DisplayStart; idx < clipper.DisplayEnd; idx++)
        {
            auto& item = items[idx];

#ifdef _DEBUG
            if (item.can_expand &&
                (item.dbl_check_if_selected + item.can_star + item.dbl_check_if_selected +
                 item.selected + item.renaming + item.starred
                 > 0))
            {
                LogError(ExcepCategory::Logic, "Invalid list item");
            }
#endif
            if (item.is_separator)
            {
                ImGui::Separator();
                continue;
            }

            ImGui::PushID(&item);
            ImGui::BeginGroup();

            ImGui::SetCursorPosX(0);
            ImGui::InvisibleButton("btn", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing()));
            
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone))
                m_hovered_item = idx;
            const bool long_delay = ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal);

            ImGui::SameLine();
            ImGui::SetCursorPosX(indent * item.depth);

            bool is_selected = item.selected || m_hovered_item == idx;

            if (item.can_expand)
            {
                if (m_hovered_item == idx)
                    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));
                is_selected = false;
                if (item.expanded) ImGui::TextUnformatted(ICON_LC_FOLDER_OPEN);
                else                ImGui::TextUnformatted(ICON_LC_FOLDER);
                if (m_hovered_item == idx)
                    ImGui::PopStyleColor();
                ImGui::SameLine();
            }
            else
            {
                if (is_selected && !item.can_expand)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));

                    if (item.dbl_check_if_selected)
                    {
                        ImGui::TextUnformatted(ICON_LC_CHECK_CHECK);
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(indent * item.depth + dbl_check_w);
                    }
                }

                if (item.can_star && idx == m_hovered_item || item.starred)
                {
                    if (item.starred) ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::Starred));
                    ImGui::TextUnformatted(ICON_LC_STAR);
                    if (item.starred) ImGui::PopStyleColor();
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
                if (item.need_focus)
                    ImGui::SetKeyboardFocusHere();
                item.need_focus = false;

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                const bool enter_pressed = ImGuiEx::InputText("##newname", &item.name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    item.name = item.name_backup;
                    item.name_backup = {};
                    item.renaming = false;
                }
                if (enter_pressed || ImGui::IsItemDeactivatedAfterEdit())
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
                ImGui::TextUnformatted(item.name.c_str(), item.name.c_str() + item.name.size());
                if (is_selected || m_hovered_item == idx)
                    ImGui::PopStyleColor();

                ImRect name_rect = ImGui::GetItemRect();
                name_rect.Min.y -= 2;
                name_rect.Max.x = ImGui::GetWindowPos().x + ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x;
                name_rect.Max.y += 3;

                if (m_hovered_item == idx)
                {
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        const auto mouse = ImGui::GetMousePos();

                        if (ico_rect.Contains(mouse) && (item.can_expand || item.can_star))
                        {
                            dirty = true;
                            dirty_item.index = idx;

                            if (item.can_star)
                            {
                                item.starred = !item.starred;
                                dirty_item.starred = true;
                            }
                            else // if (item.can_expand)
                            {
                                item.expanded = !item.expanded;
                                dirty_item.expanded = true;
                            }
                        }

                        if (!dirty && item.can_lclick)
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
                    else if (item.can_rclick && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                    {
                        dirty = true;
                        dirty_item.rclicked = true;
                        dirty_item.index = idx;
                    }
                    else if (long_delay)
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
    m_tt_life_timer = 0.5f;

    if (-1 == m_tt_index || tt_index != m_tt_index)
    {
        m_tt_index = tt_index;
        if (tt_callbacks) tt_callbacks->on_alloc_tt_data(m_tt_index);
    }

    if (tt_callbacks)
    {
        tt_callbacks->on_pre_tt(tt_index);
        if (ImGui::BeginTooltip())
        {
            tt_callbacks->on_show_tt(tt_index);
            ImGui::EndTooltip();
        }
        tt_callbacks->on_post_tt(tt_index);
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
    if (tt_callbacks) tt_callbacks->on_release_tt_data(tt_index);
    return true;
}

} // namespace ui
