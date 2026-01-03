#include "ui/options/opts_right_pane.h"

namespace ui
{

void OptRightPane::show_help_mark(const char*& out, float offset, bool same_line, const char* help_text)
{
    if (same_line)
        ImGui::SameLine();
    ImGui::SetCursorPosX(offset);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(50, 50, 50, 255));
    ImGui::TextUnformatted(" [?]");
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        out = help_text;
}

void OptRightPane::show_help_text(const char* help_text)
{
    if (help_text != nullptr)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 128, 128, 200));
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(help_text);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }
}

}
