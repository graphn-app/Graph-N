
#include "ui/options/opt_workflow_dlg.h"
#include "platform/main.h"
#include "core/managers/settings_manager.h"

namespace ui
{

OptWorkflowDlg::~OptWorkflowDlg()
{
    LogF;
}

void OptWorkflowDlg::init(OptsData& data)
{
    LogF;

    data.wf.hovered_items.resize(20);

    data.wf.numbers = g::pOptsManager->wf_widget_input_numbers_load();
    data.wf.booleans = g::pOptsManager->wf_widget_input_bools_load();
}

void OptWorkflowDlg::show(OptsData& data)
{
    const char* help_text = nullptr;

    const auto draw_selected = [](bool selected, bool& hovered, int32_t id, const char* text)
    {
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));
            ImGui::TextUnformatted(ICON_LC_CHECK_CHECK);
            ImGui::SameLine();
            ImGui::TextUnformatted(text);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushID(id);
            if (hovered)    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::SelectedItem));
            ImGui::TextUnformatted(text);
            if (hovered)    ImGui::PopStyleColor();
            ImGui::PopID();
            hovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                return true;
        }
        return false;
    };

    int32_t idx = 0;

    ///////////////
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::OptSubtitleSep));
    ImGui::TextUnformatted("Default Input Styles");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Spacing();

    ///////////////       numbers
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Numbers");
    ImGui::Spacing();

    double d_min = 5.0;
    double d_max = 142.0;

    if (draw_selected(WidgetInput::Direct == data.wf.numbers, data.wf.hovered_items[idx], idx + 1, "Input field:"))
    {
        data.wf.numbers = WidgetInput::Direct;
        g::pOptsManager->wf_widget_input_numbers_save(data.wf.numbers);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::InputDoubleSmart("##WfNumbers0", &data.wf.d_sample, 0.01, d_min, d_max, " Value: %.4f", "%.4f");


    if (draw_selected(WidgetInput::Slider == data.wf.numbers, data.wf.hovered_items[idx], idx + 1, "Slider:"))
    {
        data.wf.numbers = WidgetInput::Slider;
        g::pOptsManager->wf_widget_input_numbers_save(data.wf.numbers);
    }
    idx++;
    show_help_mark(help_text, data.input_offset_x, true, "Ctrl+click to type value.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::SliderDoubleSmart("##WfNumbers1", &data.wf.d_sample, d_min, d_max, "Value: %.4f");


    if (draw_selected(WidgetInput::Drag == data.wf.numbers, data.wf.hovered_items[idx], idx + 1, "Scrubber:"))
    {
        data.wf.numbers = WidgetInput::Drag;
        g::pOptsManager->wf_widget_input_numbers_save(data.wf.numbers);
    }
    idx++;
    show_help_mark(help_text, data.input_offset_x, true, "Ctrl+click or double click to type value.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::DragDoubleSmart("##WfNumbers2", &data.wf.d_sample, d_min, d_max, "Value: %.4f");


    ImGui::Spacing();

    ///////////////       booleans
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Booleans");
    ImGui::Spacing();


    bool b_val = !!data.wf.b_sample;

    if (draw_selected(WidgetInput::Select == data.wf.booleans, data.wf.hovered_items[idx], idx + 1, "Selectable:"))
    {
        data.wf.booleans = WidgetInput::Select;
        g::pOptsManager->wf_widget_input_bools_save(data.wf.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGuiEx::BooleanToggleLED(b_val ? "Value: on##WfBool1" : "Value: off##WfBool1", &b_val, false, false))
        data.wf.b_sample = b_val;


    if (draw_selected(WidgetInput::Toggle == data.wf.booleans, data.wf.hovered_items[idx], idx + 1, "Toggle button:"))
    {
        data.wf.booleans = WidgetInput::Toggle;
        g::pOptsManager->wf_widget_input_bools_save(data.wf.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGuiEx::BooleanButtons2("##WfBool2", b_val ? "Value: on" : "Value: off", &b_val))
        data.wf.b_sample = b_val;


    ///////////////       editor
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Editor");
    ImGui::Spacing();

    // show grid
    b_val = g::pOptsManager->wf_show_grid_load();
    if (ImGui::Checkbox("Show grid##WfShowGrid", &b_val))
        g::pOptsManager->wf_show_grid_save(b_val);
    show_help_mark(help_text, data.input_offset_x, true, "Shows or hides the background grid in the workflow editor.");

    // snap nodes
    b_val = g::pOptsManager->wf_snap_nodes_load();
    if (ImGui::Checkbox("Snap to grid##WfSnapNodes", &b_val))
        g::pOptsManager->wf_snap_nodes_save(b_val);
    show_help_mark(help_text, data.input_offset_x, true, "Snaps nodes to the grid while dragging."); // \nHold Alt to temporarily invert this behavior.


    ///////////////       help
    ImGui::Dummy(ImVec2(0.0f, 50.0f));
    show_help_text(help_text);
}

}
