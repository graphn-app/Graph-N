
#include "ui/options/opts_dlg.h"
#include "ui/options/opts_right_pane.h"
#include "ui/options/opt_comfy_dlg.h"
#include "core/managers/settings_manager.h"

namespace ui
{

OptionsDlgs::OptionsDlgs(OptPane init_opt /*= OptPane::_none*/)
	: BaseDlgWithData(VisibleObject::OptionsDlg)
{
	m_opts_data.pane.left_selected_item = init_opt;
}

OptionsDlgs::~OptionsDlgs()
{}

void OptionsDlgs::init()
{
	LogF;

	BaseDlgWithData::init();

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	data()->x = main_viewport->WorkSize.x / 2 - 300;
	data()->y = main_viewport->WorkSize.y / 2 - 300;
	data()->width = 550;
	data()->height = 450;

	m_opts_data.pane.show_inner_child = (m_opts_data.pane.left_selected_item != OptPane::_none);

	m_right_panes[OptPane::comfyui] = std::make_shared<OptComfyDlg>();
	for (size_t i = cast::to_szt(OptPane::_first); i <= cast::to_szt(OptPane::_last); i++)
	{
		m_right_panes[cast::to_enum<OptPane>(i)]->init(m_opts_data);
	}
}

DlgCode OptionsDlgs::show(bool& focused)
{
	const auto colors = g::pOptsManager->flow_get_color(VisibleObject::OptionsDlg);
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
	ImGui::BeginChild("left_pane##Opt", ImVec2(150, -FLT_MIN), true);

	if (ImGui::Selectable("ComfyUI##LeftPane", OptPane::comfyui == m_opts_data.pane.left_selected_item))
	{
		m_opts_data.pane.left_selected_item = OptPane::comfyui;
		m_opts_data.pane.show_inner_child = true;
	}

	ImGui::EndChild();
}

void OptionsDlgs::show_right_pane()
{
	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::BeginChild("right_pane##Opt");

	if (m_opts_data.pane.show_inner_child)
		ImGui::BeginChild("right_pane2##Opt", ImVec2(-FLT_MIN, -FLT_MIN), true);

	if (m_opts_data.pane.left_selected_item != OptPane::_none)
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