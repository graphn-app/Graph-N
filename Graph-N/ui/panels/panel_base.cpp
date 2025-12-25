
#include "ui/panels/panel_base.h"

namespace ui
{

bool PanelBase::begin_panel(const char* name, bool& focused, ImU32 bg_color /*= IM_COL32_BLACK*/)
{
    m_class.ClassId = cast::to_u32(data()->class_id);
    ImGui::SetNextWindowClass(&m_class);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);
    bool opened = true;
    focused = false;
    ImGui::Begin(name, &opened, ImGuiWindowFlags_NoSavedSettings);
	if (opened)
        focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    return opened;
}

DlgCode PanelBase::end_panel(bool opened)
{
	ImGui::End();
    ImGui::PopStyleColor();
    return opened ? DlgCode::progress : DlgCode::cancel;
}

} // namespace ui