
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
    show_input_styles(data);
}

void OptWorkflowDlg::show_input_styles(OptsData& data)
{
    const char* help_text = nullptr;

    const auto draw_selected = [](bool selected, bool& hovered, int32_t id, const char* text)
    {
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::opt_selected_item));
            ImGui::TextUnformatted(ICON_LC_CHECK_CHECK);
            ImGui::SameLine();
            ImGui::TextUnformatted(text);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushID(id);
            if (hovered)    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::opt_selected_item));
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
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::opt_subtitle_sep));
    ImGui::TextUnformatted("Default Input Styles");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Spacing();

    ///////////////       numbers
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), "Numbers");
    ImGui::Spacing();

    double d_min = 5.0;
    double d_max = 142.0;

    if (draw_selected(WidgetInput::direct == data.wf.numbers, data.wf.hovered_items[idx], idx + 1, "Input Field:"))
    {
        data.wf.numbers = WidgetInput::direct;
        g::pOptsManager->wf_widget_input_numbers_save(data.wf.numbers);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::InputDoubleSmart("##WfNumbers0", &data.wf.d_sample, 0.01, d_min, d_max, " Value: %.4f", "%.4f");


    if (draw_selected(WidgetInput::slider == data.wf.numbers, data.wf.hovered_items[idx], idx + 1, "Slider:"))
    {
        data.wf.numbers = WidgetInput::slider;
        g::pOptsManager->wf_widget_input_numbers_save(data.wf.numbers);
    }
    idx++;
    show_help_mark(help_text, data.input_offset_x, true, "Ctrl+Click to type value.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::SliderDoubleSmart("##WfNumbers1", &data.wf.d_sample, d_min, d_max, "Value: %.4f");


    if (draw_selected(WidgetInput::drag == data.wf.numbers, data.wf.hovered_items[idx], idx + 1, "Scrubber:"))
    {
        data.wf.numbers = WidgetInput::drag;
        g::pOptsManager->wf_widget_input_numbers_save(data.wf.numbers);
    }
    idx++;
    show_help_mark(help_text, data.input_offset_x, true, "Ctrl+Click or Double Click to type value.");
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGuiEx::DragDoubleSmart("##WfNumbers2", &data.wf.d_sample, d_min, d_max, "Value: %.4f");


    ImGui::Spacing();

    ///////////////       booleans
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), "Booleans");
    ImGui::Spacing();


    bool b_val = !!data.wf.b_sample;

/*
    if (draw_selected(WidgetInput::switcher == data.wf.booleans, data.wf.hovered_items[idx], idx + 1, "Switch:"))
    {
        data.wf.booleans = WidgetInput::switcher;
        g::pOptsManager->wf_set_widget_input_bools(data.wf.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGuiEx::BooleanSwitch("##WfBool0", b_val ? "Value: on" : "Value: off", &b_val))
        data.wf.b_sample = b_val;
*/

    if (draw_selected(WidgetInput::select == data.wf.booleans, data.wf.hovered_items[idx], idx + 1, "Selectable:"))
    {
        data.wf.booleans = WidgetInput::select;
        g::pOptsManager->wf_widget_input_bools_save(data.wf.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGuiEx::BooleanToggleLED(b_val ? "Value: on##WfBool1" : "Value: off##WfBool1", &b_val, false, false))
        data.wf.b_sample = b_val;


    if (draw_selected(WidgetInput::toggle == data.wf.booleans, data.wf.hovered_items[idx], idx + 1, "Toggle Button:"))
    {
        data.wf.booleans = WidgetInput::toggle;
        g::pOptsManager->wf_widget_input_bools_save(data.wf.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGuiEx::BooleanButtons2("##WfBool2", b_val ? "Value: on" : "Value: off", &b_val))
        data.wf.b_sample = b_val;


    ///////////////       help
    ImGui::Dummy(ImVec2(0.0f, 50.0f));
    show_help_text(help_text);
}

}
