
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

OptionsDlgs::~OptionsDlgs() = default;

void OptionsDlgs::init()
{
    LogF;

    BaseDlgWithData::init();

    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    data()->x = main_viewport->WorkSize.x / 2 - 300;
    data()->y = main_viewport->WorkSize.y / 2 - 300;
    data()->width = 550;
    data()->height = 450;

    data()->clr_title = g::pOptsManager->flow_get_color(VisibleObject::OptionsDlg);
    data()->clr_bg = g::pOptsManager->flow_get_color(VisibleObject::OptionsBg);

    m_opts_data.pane.show_inner_child = (m_opts_data.pane.left_selected_item != OptPane::_none);

    m_right_panes[OptPane::comfyui] = std::make_unique<OptComfyDlg>();
    for (uint64_t i = cast::to_u64(OptPane::_first); i <= cast::to_u64(OptPane::_last); i++)
    {
        m_right_panes[cast::to_enum<OptPane>(i)]->init(m_opts_data);
    }
}

DlgCode OptionsDlgs::show(bool& focused)
{
    ImGui::PushDlgTitleStyles(data()->clr_title);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, data()->clr_bg);

    set_focus();
    ImGui::SetNextWindowPos(ImVec2(data()->x, data()->y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(data()->width, data()->height), ImGuiCond_Always);

    bool opened = true;
    if (ImGui::Begin("Options##Opt", &opened, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking))
    {
        focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

        show_left_pane();
        show_right_pane();
    }
    return cleanup_and_exit(opened);
}

void OptionsDlgs::show_left_pane()
{
    if (ImGui::BeginChild("left_pane##Opt", ImVec2(150, -FLT_MIN), true))
    {
        if (ImGui::Selectable("ComfyUI##LeftPane", OptPane::comfyui == m_opts_data.pane.left_selected_item))
        {
            m_opts_data.pane.left_selected_item = OptPane::comfyui;
            m_opts_data.pane.show_inner_child = true;
        }
    }
    ImGui::EndChild();
}

void OptionsDlgs::show_right_pane()
{
    ImGui::SameLine();
    ImGui::BeginGroup();
    if (ImGui::BeginChild("right_pane##Opt"))
    {
        if (m_opts_data.pane.show_inner_child)
        {
            if (ImGui::BeginChild("right_pane2##Opt", ImVec2(-FLT_MIN, -FLT_MIN), true))
            {
                if (m_opts_data.pane.left_selected_item != OptPane::_none)
                {
                    try
                    {
                        m_right_panes[m_opts_data.pane.left_selected_item]->show(m_opts_data);
                    }
                    catch (const std::exception& ex)
                    {
                        LogExcep(ExcepCategory::ui, "Options: " << ex.what());
                    }
                }

            }
            ImGui::EndChild();
        }
    }
    ImGui::EndChild();
    ImGui::EndGroup();

}

}
