
#include "ui/options/opts_dlg.h"
#include "ui/options/opts_right_pane.h"
#include "ui/options/opt_comfyui_dlg.h"
#include "ui/options/opt_workflow_dlg.h"
#include "ui/options/opt_stats_dlg.h"
#include "core/managers/settings_manager.h"

namespace ui
{

OptionsDlgs::OptionsDlgs(OptPane init_opt /*= OptPane::_none*/)
    : BaseDlgWithData(VisibleObject::OptionsDlg)
{
    m_opts_data.pane.left_selected_item = init_opt;
}

OptionsDlgs::~OptionsDlgs()
{
    LogF;
}

void OptionsDlgs::init()
{
    LogF;

    BaseDlgWithData::init();

    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    data()->x = main_viewport->WorkSize.x / 2 - 300;
    data()->y = main_viewport->WorkSize.y / 2 - 300;
    data()->width = 650;
    data()->height = 680;

    data()->clr_title = g::pOptsManager->color_load(AppColor::opt_dlg_title_bg);
    data()->clr_bg = g::pOptsManager->color_load(AppColor::opt_dlg_bg);

    m_opts_data.buffer_i.resize(cast::to_szt(OptPane::_count));
    m_opts_data.buffer_s.resize(cast::to_szt(OptPane::_count));

    m_opts_data.comfyui.buffer_s = &m_opts_data.buffer_s[cast::to_szt(OptPane::comfyui)];
    m_opts_data.comfyui.buffer_i = &m_opts_data.buffer_i[cast::to_szt(OptPane::comfyui)];

    m_opts_data.stats.buffer_s = &m_opts_data.buffer_s[cast::to_szt(OptPane::stats)];
    m_opts_data.stats.buffer_i = &m_opts_data.buffer_i[cast::to_szt(OptPane::stats)];

    m_opts_data.pane.show_inner_child = (m_opts_data.pane.left_selected_item != OptPane::_none);

    m_right_panes[OptPane::comfyui] = std::make_unique<OptComfyDlg>();
    m_right_panes[OptPane::workflow] = std::make_unique<OptWorkflowDlg>();
    m_right_panes[OptPane::stats] = std::make_unique<OptStatsDlg>();

    for (uint64_t i = cast::to_u64(OptPane::_first); i <= cast::to_u64(OptPane::_last); i++)
    {
        m_right_panes[cast::to_enum<OptPane>(i)]->init(m_opts_data);
    }
}

void OptionsDlgs::select_right_pane(OptPane pane)
{
    m_opts_data.pane.left_selected_item = pane;
}

DlgCode OptionsDlgs::show(bool& focused)
{
    ImGuiEx::PushDlgTitleStyles(data()->clr_title);
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
        const auto avail_h = ImGui::GetContentRegionAvail().y;

        if (ImGui::Selectable(APP_COMFYUI "##LeftPane", OptPane::comfyui == m_opts_data.pane.left_selected_item))
        {
            m_opts_data.pane.left_selected_item = OptPane::comfyui;
            m_opts_data.pane.show_inner_child = true;
        }
        if (ImGui::Selectable("Workflow##LeftPane", OptPane::workflow == m_opts_data.pane.left_selected_item))
        {
            m_opts_data.pane.left_selected_item = OptPane::workflow;
            m_opts_data.pane.show_inner_child = true;
        }
        if (ImGui::Selectable("Stats##LeftPane", OptPane::stats == m_opts_data.pane.left_selected_item))
        {
            m_opts_data.pane.left_selected_item = OptPane::stats;
            m_opts_data.pane.show_inner_child = true;
        }
        
        ImGui::SetCursorPosY(avail_h - ImGui::GetTextLineHeight());
        ImGui::PushStyleColor(ImGuiCol_Button, g::pOptsManager->color_load(AppColor::restart_button));
        const bool restart = ImGui::Button("Restart " APP_COMFYUI, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
        ImGui::PopStyleColor();
        if (restart)
        {
            // restart
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
