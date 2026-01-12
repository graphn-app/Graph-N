
#pragma once
#include "ui/base_dlg.h"

namespace ui
{

struct PanelCommonData : public DlgData
{
    const float button_width = 30.0f;
    const float grouped_btn_spacing_x = 1.0f;
    const float ungrouped_btn_spacing_x = 2.0f;
};

template <typename DataType>
class PanelBase : public BaseDlgWithData<DataType>
{
public:
    PanelBase(VisibleObject obj, const char* name, const char* id)
        : BaseDlgWithData(obj)
    {
        m_panel_name = fmt::format("{}##{}", name, id);
    }

    ~PanelBase() override = default;

    const char* get_panel_name() const { return m_panel_name.c_str(); }

protected:
    bool begin_panel(bool& opened, bool& focused, ImU32 bg_color = IM_COL32_BLACK)
    {
        // m_class.ClassId = cast::to_u32(data()->class_id);
        // ImGui::SetNextWindowClass(&m_class);

        opened = true;
        focused = false;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);
        const bool expanded = ImGui::Begin(m_panel_name.c_str(), &opened);
        if (expanded)
            focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        return expanded;
    }

    DlgCode end_panel(bool opened)
    {
        ImGui::End();
        ImGui::PopStyleColor();
        return opened ? DlgCode::progress : DlgCode::cancel;
    }

    void show_empty_panel_descr(const char* header_descr, const std::vector<const char*>& titles, const std::vector<const char*>& subtitles)
    {
        ImGui::Spacing();

        // --- Static Description (Top of the window) ---
        // Make it slightly dimmed to act as a "subtitle"
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        ImGui::TextWrapped(header_descr);
        ImGui::PopStyleColor();

        ImGui::Separator(); // Visually separate help from state

        // --- Center the "Empty State" in the remaining space ---

        // Calculate total height to center vertically
        const float line_h = ImGui::GetTextLineHeight();
        const float spacing = ImGui::GetStyle().ItemSpacing.y;
        // Icon + Title + Spacing + Subtitle + Help1 + Help2
        const float total_h = line_h * 5 + spacing * 4;

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        float start_y = (avail.y - total_h) * 0.5f;
        if (start_y < 20.0f)
            start_y = 20.0f; // Safety margin

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + start_y);

        // Helper lambda for centered text
        const auto text_centered = [&](const char* text, bool disabled)
        {
            float w = ImGui::CalcTextSize(text).x;
            ImGui::SetCursorPosX((avail.x - w) * 0.5f);
            if (disabled)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::TextUnformatted(text);
            if (disabled)
                ImGui::PopStyleColor();
        };

        // Draw Content
        for (auto& title : titles)
        {
            text_centered(title, false);
        }

        ImGui::Dummy(ImVec2(0, spacing * 2)); // Extra gap

        // Subtitles (Grey)
        for (auto& subtitle : subtitles)
        {
            text_centered(subtitle, true);
        }
    }

private:
    std::string m_panel_name;

    //  ImGuiWindowClass m_class;
};


} // namespace ui
