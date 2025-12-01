
#include "platform/main.h"
#include "core/base/version.h"

namespace platform
{

void MainRoutine::draw_app_version(ImDrawList* drawList)
{
	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	const ImVec2 line1(10, main_viewport->WorkSize.y - 30);
	const ImVec2 line2(10, main_viewport->WorkSize.y - 15);

	constexpr auto color = IM_COL32_WHITE;
	static const std::string version = ver::ver_as_string_no_prepended_zeros();

	drawList->AddText(m_font, 15, line1, color, APP_CAPTION);
	drawList->AddText(m_font, 15, line2, color, version.c_str());
}

} // namespace platform
