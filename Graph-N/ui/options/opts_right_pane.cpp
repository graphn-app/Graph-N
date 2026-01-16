#include "ui/options/opts_right_pane.h"
#include "core/managers/settings_manager.h"

namespace ui
{

OptRightPane::~OptRightPane()
{
    LogF;
}

void OptRightPane::show_help_mark(const char*& out, float offset, bool same_line, const char* help_text)
{
    if (same_line)
        ImGui::SameLine();
    ImGui::SetCursorPosX(offset);
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::OptHelpMark));
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
        ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::OptHelpText));
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(help_text);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }
}

}
