
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

private:
    std::string m_panel_id;
    std::string m_panel_name;
    std::string m_panel_name_shortened;
    float m_panel_name_w = 0.0f;
    float m_panel_w = 0.0f;
    bool m_need_focus = false;
    //  ImGuiWindowClass m_class;
};

class PanelEmptyWithDescr
{
public:
    PanelEmptyWithDescr() = default;
    ~PanelEmptyWithDescr() = default;

    NO_COPY_MOVE(PanelEmptyWithDescr);

    void show_empty_panel_descr(
        const char* header_descr,
        const std::vector<const char*>& titles,
        const std::vector<const char*>& subtitles,
        const char* url = nullptr, std::function<void()> url_handler = nullptr);
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

class PanelToolbar
{
public:
    PanelToolbar() = default;
    ~PanelToolbar() = default;

    NO_COPY_MOVE(PanelToolbar);

    void draw_toolbar();

    struct Button
    {
        const char* icon = nullptr;
        float ico_width = 0.0f;
        const char* tooltip = nullptr;
        bool long_indent_after = false;
        bool enabled = true;
        std::function<void(Button&)> on_click = nullptr;
        std::function<void(Button&)> on_preview = nullptr;
        std::function<void(Button&)> on_postview = nullptr;
    };

    // handler can return new icon and tooltip
    void add_button(Button&& button);
    void finalize_toolbar();

private:
    std::vector<Button> m_buttons;
    ImU32 m_clr_hover = 0;
    ImU32 m_clr_pressed = 0;
    float m_toolbar_w = 0.0f;
};


} // namespace ui
