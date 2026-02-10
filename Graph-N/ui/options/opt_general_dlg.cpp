
#include "ui/options/opt_general_dlg.h"
#include "core/managers/settings_manager.h"

namespace ui
{

OptGeneralDlg::~OptGeneralDlg()
{
    LogF;
}

void OptGeneralDlg::init(OptsData& /*data*/)
{
    LogF;
}

void OptGeneralDlg::show(OptsData& data)
{
    const char* help_text = nullptr;
    bool b_val = true;

    ///////////////       common
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Common");
    ImGui::Spacing();

    // vsync
    b_val = g::pOptsManager->general_vsync_load();
    if (ImGui::Checkbox("Enable VSync##WfShowGrid", &b_val))
        g::pOptsManager->general_vsync_save(b_val);
    show_help_mark(help_text, data.input_offset_x, true, "Synchronizes the frame rate with your monitor's refresh rate.\nKeep this enabled for a smooth, tear-free image.\nDisable it only if you have a weak GPU and experience stuttering, as it allows the interface to run at its own pace.\n\nNote: Disabling vsync will show artificially high FPS and increase GPU load; refer to Latency for real performance.");


    ///////////////       help
    ImGui::Dummy(ImVec2(0.0f, 50.0f));
    show_help_text(help_text);
}

}
