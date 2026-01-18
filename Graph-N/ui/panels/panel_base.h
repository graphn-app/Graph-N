
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
    PanelBase(DlgObj obj, const char* name, const char* shortened, const char* id, bool need_focus = false)
        : BaseDlgWithData(obj)
        , m_need_focus(need_focus)
    {
        m_panel_id = id;
        m_panel_name = fmt::format("{}###{}", name, id);
        m_panel_name_w = ImGui::CalcTextSize(name).x;
        m_panel_name_shortened = fmt::format("{}###{}", shortened, id);
    }

    ~PanelBase() override = default;

    const char* get_panel_name() const { return m_panel_name.c_str(); }
    const char* get_panel_id() const { return m_panel_id.c_str(); }

protected:
    bool begin_panel(bool& opened, bool& focused, ImU32 bg_color, bool save_pos = true)
    {
        // m_class.ClassId = cast::to_u32(data()->class_id);
        // ImGui::SetNextWindowClass(&m_class);

        const std::string* name = &m_panel_name;
        if (m_panel_w > 0 && m_panel_w < m_panel_name_w + 60.0f)
            name = &m_panel_name_shortened;

        opened = true;
        focused = false;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);
        const bool expanded = ImGui::Begin(name->c_str(), &opened, save_pos ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoSavedSettings);
        m_panel_w = ImGui::GetWindowWidth();

        if (expanded && m_need_focus)
            focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        return expanded;
    }

    DlgCode end_panel(bool opened)
    {
        ImGui::End();
        ImGui::PopStyleColor();
        return opened ? DlgCode::Progress : DlgCode::Cancel;
    }

    void show_empty_panel_descr(const char* header_descr, const std::vector<const char*>& titles, const std::vector<const char*>& subtitles,
        const char* url = nullptr, std::function<void()> url_handler = nullptr)
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
        const float total_h = line_h * 7 + spacing * 4;

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        float start_y = (avail.y - total_h) * 0.5f;
        if (start_y < 20.0f)
            start_y = 20.0f; // Safety margin

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + start_y);

        // Helper lambda for centered text
        const auto text_centered = [&](const char* text, bool disabled, bool as_url)
        {
            bool click = false;
            const float w = ImGui::CalcTextSize(text).x;
            ImGui::SetCursorPosX((avail.x - w) * 0.5f);
            if (disabled)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            if (as_url)
                click = ImGuiEx::UrlButton(url, g::pOptsManager->color_load(AppColor::UrlTextDefault), g::pOptsManager->color_load(AppColor::UrlTextHovered), nullptr);
            else
                ImGui::TextUnformatted(text);
            if (disabled)
                ImGui::PopStyleColor();
            return click;
        };

        // Draw Content
        for (auto& title : titles)
        {
            text_centered(title, false, false);
        }

        ImGui::Dummy(ImVec2(0, spacing * 2)); // Extra gap

        // Subtitles (Grey)
        for (auto& subtitle : subtitles)
        {
            text_centered(subtitle, true, false);
        }

        if (url != nullptr && url_handler != nullptr)
        {
            ImGui::Spacing();
            if (text_centered(url, false, true))
                url_handler();
        }
    }

private:
    std::string m_panel_id;
    std::string m_panel_name;
    std::string m_panel_name_shortened;
    float m_panel_name_w = 0.0f;
    float m_panel_w = 0.0f;
    bool m_need_focus = false;
    //  ImGuiWindowClass m_class;
};

class PanelWithList
{
public:
    PanelWithList() = default;
    ~PanelWithList() = default;

    NO_COPY_MOVE(PanelWithList);

    struct TtCallbacks
    {
        std::function<void(int32_t)> on_alloc_tt_data;
        std::function<void(int32_t)> on_release_tt_data;
        std::function<void(int32_t)> on_show_tt;
        std::function<void(int32_t)> on_pre_tt;
        std::function<void(int32_t)> on_post_tt;
    };

    // returns true if dirty
    // draw_boudnded_rect applicable only for expandable items
    bool draw_list(std::vector<PanelListItem>& items, PanelItemState& dirty_item, 
        const TtCallbacks* tt_callbacks = nullptr);

private:
    void go_tooltip(int32_t tt_index, const TtCallbacks* tt_callbacks);
    bool release_tooltip(int32_t tt_index, const TtCallbacks* tt_callbacks);

private:
    int32_t m_hovered_item = -1;
    std::vector<int32_t> m_selected_items;
    float m_tt_life_timer = 0.0f;
    int32_t m_tt_index = -1;
};


} // namespace ui
