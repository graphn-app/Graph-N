/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_media_widget.h"
#include "ui/graph-n_flow.h"
#include "core/workflow/node_instance.h"

namespace ui
{

void CustomMediaWidget::draw_text_widget_from_output(OutputData& out)
{
    if (out.item.text_info.empty())
        return;

    ImGui::SetCursorPosX(m_start_pos);

    const ImVec2 text_size = ImGui::CalcTextSize(out.item.text_info.c_str(), nullptr, false, m_widget_width);
    const bool is_truncated = text_size.y > WIDGET_OUT_TEXT_MAX_H;
    const float final_height = std::min(text_size.y, WIDGET_OUT_TEXT_MAX_H);
    const ImVec2 start_pos = ImGui::GetCursorScreenPos();

    ImGui::Dummy(ImVec2(m_widget_width, final_height));
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        g::pFlow->txt_panel_create(m_node->get_workflow_id(), m_node, nullptr, &out.item.text_info, nullptr);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec4 clip_rect(start_pos.x, start_pos.y, start_pos.x + m_widget_width, start_pos.y + final_height);
    draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize(), start_pos, ImGui::GetColorU32(ImGuiCol_Text), out.item.text_info.c_str(), nullptr, m_widget_width, &clip_rect);

    if (is_truncated)
    {
        const char* ellipsis = "[...]";
        const ImVec2 ell_size = ImGui::CalcTextSize(ellipsis);
        draw_list->AddText(ImVec2(m_start_pos, start_pos.y + final_height), ImGui::GetColorU32(ImGuiCol_Text), ellipsis);
    }

    const float button_size = ImGui::GetFrameHeight();
    ImGui::SetCursorPosX(m_start_pos + m_widget_width - button_size * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f); // top padding

    const ImGuiID btn_id = ImGui::GetID(out.item.text_info.c_str()); 
    static std::map<ImGuiID, double> copy_timestamps;
    const bool is_copied = (ImGui::GetTime() - copy_timestamps[btn_id]) < 1.0;
    const char* icon = is_copied ? ICON_LC_CHECK : ICON_LC_CLIPBOARD_COPY;

    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, is_copied ? IM_COL32(50, 205, 50, 255) : IM_COL32(255, 255, 255, 128));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiEx::decrease_brightness(ImGui::GetColorU32(m_node->m_clr_node)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(102, 102, 102, 128));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));

    if (ImGui::Button(icon, ImVec2(button_size, button_size)))
    {
        ImGui::SetClipboardText(out.item.text_info.c_str());
        copy_timestamps[btn_id] = ImGui::GetTime();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
}

void CustomMediaWidget::copy_text_output(wid wf_id, iid node_id)
{
    const auto it = m_outputs.find(std::make_pair(wf_id, node_id));
    if (it != m_outputs.end() && NodeObjFlags::CustomMediaText == it->second.media_type && !it->second.item.text_info.empty())
    {
        ImGui::SetClipboardText(it->second.item.text_info.c_str());
    }
}

void CustomMediaWidget::view_text_output(wid wf_id, iid node_id)
{
    const auto it = m_outputs.find(std::make_pair(wf_id, node_id));
    if (it != m_outputs.end() && NodeObjFlags::CustomMediaText == it->second.media_type && !it->second.item.text_info.empty())
    {
        const auto* wf = g::pWfsManager->access(wf_id);
        LogRAssert(wf, "wf:" << wf_id << " not found", );
        auto node = wf->get_node_by_instance(node_id);
        LogRAssert(node, "wf:" << wf_id << ", n:" << node_id << " not found", );

        g::pFlow->txt_panel_create(wf_id, node, nullptr, &it->second.item.text_info, nullptr);
    }
}

} // namespace ui
