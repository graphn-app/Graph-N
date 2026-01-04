
#include "ui/options/opt_workflow_dlg.h"
#include "platform/main.h"
#include "core/managers/settings_manager.h"

namespace ui
{

void OptWorkflowDlg::init(OptsData& data)
{
    LogF;

    data.wf.hovered_items.resize(10);

    data.wf.numbers = g::pOptsManager->wf_get_widget_input_numbers();
    data.wf.booleans = g::pOptsManager->wf_get_widget_input_bools();
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
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 255));
            ImGui::TextUnformatted(ICON_LC_CHECK_CHECK);
            ImGui::SameLine();
            ImGui::TextUnformatted(text);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.x);
            ImGui::PushID(id);
            if (hovered)    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 255));
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
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 128, 128, 255));
    ImGui::TextUnformatted("Default Input Styles");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Spacing();

    ///////////////       numbers
    ImGui::Separator(data.separator_clr, "Numbers");
    ImGui::Spacing();

    double d_min = 5.0;
    double d_max = 142.0;

    if (draw_selected(WidgetInput::direct == data.wf.numbers, data.wf.hovered_items[idx], idx + 1, "Input Field:"))
    {
        data.wf.numbers = WidgetInput::direct;
        g::pOptsManager->wf_set_widget_input_numbers(data.wf.numbers);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputDoubleSmart("##WfNumbers0", &data.wf.d_sample, 0.01, d_min, d_max, "Value: %.4f", "%.4f");


    if (draw_selected(WidgetInput::slider == data.wf.numbers, data.wf.hovered_items[idx], idx + 1, "Slider:"))
    {
        data.wf.numbers = WidgetInput::slider;
        g::pOptsManager->wf_set_widget_input_numbers(data.wf.numbers);
    }
    idx++;
    show_help_mark(help_text, data.input_offset_x, true, "Ctrl+Click to type value.");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::SliderScalar("##WfNumbers1", ImGuiDataType_Double, &data.wf.d_sample, &d_min, &d_max, "Value: %.4f");

    ImGui::Spacing();

    ///////////////       booleans
    ImGui::Separator(data.separator_clr, "Booleans");
    ImGui::Spacing();


    uint8_t b_min = 0;
    uint8_t b_max = 1;
    bool b_val = !!data.wf.b_sample;
    const char* b_items[] = { "Value: off", "Value: on" };

    if (draw_selected(WidgetInput::slider == data.wf.booleans, data.wf.hovered_items[idx], idx + 1, "Toggle switch:"))
    {
        data.wf.booleans = WidgetInput::slider;
        g::pOptsManager->wf_set_widget_input_bools(data.wf.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::SliderScalar("##WfBool0", ImGuiDataType_U8, &data.wf.b_sample, &b_min, &b_max, data.wf.b_sample ? "Value: on" : "Value: off", ImGuiSliderFlags_NoInput);

    if (draw_selected(WidgetInput::checkbox == data.wf.booleans, data.wf.hovered_items[idx], idx + 1, "Checkbox:"))
    {
        data.wf.booleans = WidgetInput::checkbox;
        g::pOptsManager->wf_set_widget_input_bools(data.wf.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::Checkbox(data.wf.b_sample ? "Value: on##WfBool1" : "Value: off##WfBool1", &b_val))
        data.wf.b_sample = b_val;


    if (draw_selected(WidgetInput::radio == data.wf.booleans, data.wf.hovered_items[idx], idx + 1, "Radio group:"))
    {
        data.wf.booleans = WidgetInput::radio;
        g::pOptsManager->wf_set_widget_input_bools(data.wf.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    const float second_radio = (ImGui::GetContentRegionAvail().x / 2);
    if (ImGui::RadioButton("Value: on##WfBool2", b_val))
        data.wf.b_sample = 1;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f + second_radio);
    if (ImGui::RadioButton("Value: off##WfBool2", !b_val))
        data.wf.b_sample = 0;

    if (draw_selected(WidgetInput::combo == data.wf.booleans, data.wf.hovered_items[idx], idx + 1, "Dropdown:"))
    {
        data.wf.booleans = WidgetInput::combo;
        g::pOptsManager->wf_set_widget_input_bools(data.wf.booleans);
    }
    idx++;
    ImGui::SameLine();
    ImGui::SetCursorPosX(data.input_offset_x + 29.0f);
    ImGui::SetNextItemWidth(-FLT_MIN);
    int current = (b_val ? 1 : 0);
    if (ImGui::Combo("##WfBool3", &current, b_items, 2))
        data.wf.b_sample = current ? 1 : 0;


    ///////////////       help
    ImGui::Dummy(ImVec2(0.0f, 50.0f));
    show_help_text(help_text);
}

}
