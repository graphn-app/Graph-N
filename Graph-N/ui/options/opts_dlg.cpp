
#include "ui/options/opts_dlg.h"
#include "ui/options/opts_right_pane.h"
#include "ui/options/opt_comfy_dlg.h"
#include "core/managers/settings_manager.h"

namespace ui
{

OptionsDlgs::OptionsDlgs()
	: BaseDlg(DlgObject::Options)
{}

OptionsDlgs::~OptionsDlgs()
{}

void OptionsDlgs::init()
{
	LogF;

	BaseDlg::init();

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	data()->x = main_viewport->WorkSize.x / 2 - 300;
	data()->y = main_viewport->WorkSize.y / 2 - 300;;
	data()->width = 650;
	data()->height = 400;

	m_right_panes[PaneType::comfyui] = std::make_shared<OptComfyDlg>();
	for (size_t i = cast::to_szt(PaneType::_first); i <= cast::to_szt(PaneType::_last); i++)
	{
		m_right_panes[cast::to_enum<PaneType>(i)]->init(m_opts_data);
	}
}

DlgCode OptionsDlgs::show(bool& focused)
{
	const auto colors = g::pOptsManager->get_window_title_color(DlgObject::Options);
	ImGui::PushDlgTitleStyles(colors);

	set_focus();
	bool opened = true;
	ImGui::SetNextWindowPos(ImVec2(data()->x, data()->y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(data()->width, data()->height), ImGuiCond_Always);

	ImGui::Begin("Options##Opt", &opened, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
	if (!opened)
		return cleanup_and_exit(true, true, DlgCode::cancel);
	focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

	show_left_pane();
	show_right_pane();

	return cleanup_and_exit(true, true, DlgCode::progress);
}

void OptionsDlgs::show_left_pane()
{
	ImGui::BeginChild("left_pane##Opt", ImVec2(200, m_opts_data.pane.height), true);
	ImGui::SetNextWindowPos(ImVec2(data()->x, data()->y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(data()->width, data()->height), ImGuiCond_FirstUseEver);

	if (ImGui::Selectable("ComfyUI##LeftPane", PaneType::comfyui == m_opts_data.pane.left_selected_item))
	{
		m_opts_data.pane.left_selected_item = PaneType::comfyui;
		m_opts_data.pane.height = 450;
		m_opts_data.pane.width = 370;
		m_opts_data.pane.show_inner_child = true;
	}

	if (m_opts_data.pane.left_selected_item != PaneType::_none)
	{
		m_opts_data.pane.width = m_opts_data.pane.width + 200 + 50;
		m_opts_data.pane.height = m_opts_data.pane.height + 70;
	}

	ImGui::EndChild();
}

void OptionsDlgs::show_right_pane()
{
	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::BeginChild("right_pane##Opt", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us

	if (m_opts_data.pane.show_inner_child)
		ImGui::BeginChild("right_pane2##Opt", ImVec2(m_opts_data.pane.width, m_opts_data.pane.height), true);

	if (m_opts_data.pane.left_selected_item != PaneType::_none)
	{
		try
		{
			m_right_panes[m_opts_data.pane.left_selected_item]->show(m_opts_data);
		}
		catch (const std::exception& ex)
		{
			LogExcep("Options: " << ex.what());
		}
	}

	if (m_opts_data.pane.show_inner_child)
		ImGui::EndChild();

	ImGui::EndChild();
	ImGui::EndGroup();

}

}