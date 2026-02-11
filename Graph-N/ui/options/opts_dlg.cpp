
#include "ui/options/opts_dlg.h"
#include "ui/options/opts_right_pane.h"
#include "core/managers/settings_manager.h"

namespace ui
{

OptionsDlgs::OptionsDlgs()
    : BaseDlgWithData(DlgObj::OptionsDlg)
{
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
    data()->y = main_viewport->WorkSize.y / 2 - 400;
    data()->width = 800;
    data()->height = 700;

    data()->clr_title = g::pOptsManager->color_load(AppColor::OptDlgTitleBg);
    data()->clr_bg = g::pOptsManager->color_load(AppColor::OptDlgBg);

    m_opts_data.left_pane_w = 150.0f;

    m_right_pane = std::make_unique<OptRightPane>();
    m_right_pane->init(m_opts_data);
}

void OptionsDlgs::select_right_pane(OptPane pane)
{
    m_opts_data.left_selected_item = cast::to_szt(pane);
    m_opts_data.show_inner_child = true;
}

DlgCode OptionsDlgs::show(bool& /*focused*/)
{
    ImGuiEx::PushDlgTitleStyles(data()->clr_title);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, data()->clr_bg);

    ImGui::SetNextWindowPos(ImVec2(data()->x, data()->y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(data()->width, data()->height), ImGuiCond_Always);

    bool opened = true;
    if (ImGui::Begin(DlgObjID[cast::to_szt(DlgObj::OptionsDlg)], &opened,
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking))
    {
        show_left_pane();
        m_opts_data.show_inner_child = (m_opts_data.left_selected_item >= cast::to_szt(OptPane::_first) && m_opts_data.left_selected_item <= cast::to_szt(OptPane::_last));
        show_right_pane();

       // if (m_opts_data.enable_always_on_top)
       //     ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    }
    return cleanup_and_exit(opened);
}

void OptionsDlgs::show_left_pane()
{
    constexpr const char* opt_list[cast::to_szt(OptPane::_count)] =
    {
        "Application",
        "Connection",
        "Local Server",
        "Interface",
        "Diagnostics",
        "Key Bindings",
    };

    if (ImGui::BeginChild(DlgObjID[cast::to_szt(DlgObj::OptLeftPane)], ImVec2(m_opts_data.left_pane_w, -FLT_MIN), true))
    {
        const auto avail_h = ImGui::GetContentRegionAvail().y;

        for (size_t i = cast::to_szt(OptPane::_first); i <= cast::to_szt(OptPane::_last); i++)
        {
            if (ImGui::Selectable(opt_list[i], i == m_opts_data.left_selected_item))
            {
                m_opts_data.left_selected_item = i;
            }
        }
        
        ImGui::SetCursorPosY(avail_h - ImGui::GetTextLineHeight());
        ImGui::PushStyleColor(ImGuiCol_Button, g::pOptsManager->color_load(AppColor::RestartButton));
        const bool restart = ImGui::Button("Restart " APP_COMFYUI, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
        ImGui::PopStyleColor();
        if (restart)
        {
            // TODO
            // restart
        }
    }
    ImGui::EndChild();
}

void OptionsDlgs::show_right_pane()
{
    ImGui::SameLine();
    ImGui::BeginGroup();
    if (ImGui::BeginChild(DlgObjID[cast::to_szt(DlgObj::OptRightPane)]))
    {
        if (m_opts_data.show_inner_child)
        {
            if (ImGui::BeginChild(DlgObjID[cast::to_szt(DlgObj::OptRightPane_Sub)], ImVec2(-FLT_MIN, -FLT_MIN), true))
            {
                try
                {
                    m_right_pane->init_sizes();
                    m_right_pane->show(m_opts_data);
                }
                catch (const std::exception& ex)
                {
                    LogExcep(ExcepCategory::UI, "Options: " << ex.what());
                }
            }
            ImGui::EndChild();
        }
    }
    ImGui::EndChild();
    ImGui::EndGroup();
}

}
