
#pragma once
#include "ui/base_dlg.h"
#include "ui/graph-n_flow.h"

namespace ui
{

struct PanelCommonData : public DlgData
{
    const float button_width = 30.0f;
    const float grouped_btn_spacing_x = 1.0f;
    const float ungrouped_btn_spacing_x = 2.0f;

    ImU32 clr_active_wf = 0;
    ImU32 clr_active_hov_wf = 0;
};

template <typename DataType>
class PanelBase : public BaseDlgWithData<DataType>
{
public:
    PanelBase(DlgObj obj, const std::string& name, const std::string& id = "", bool need_focus = false)
        : BaseDlgWithData(obj)
        , m_need_focus(need_focus)
    {
        m_id = id.empty() ? DlgObjID[cast::to_szt(obj)] : id;
        if (m_id.find("###") == std::string::npos)
            m_id = "###" + m_id;
        m_icon = g::pFlow->get_panel_icon(obj);
        m_name = name;
        build_names();
    }

    ~PanelBase() override = default;

    inline const char* get_panel_name() const { return m_name.c_str(); }
    inline const char* get_panel_name_full() const { return m_name_full.c_str(); }
    inline const char* get_panel_id() const { return m_id.c_str(); }

    inline void set_focused() { m_activate = true; }

protected:
    inline void set_name(const std::string& new_name) { m_name = new_name; build_names(); }
    inline void set_id(const std::string& new_id)
    {
        m_id = new_id;
        if (m_id.find("###") == std::string::npos)
            m_id = "###" + m_id;
        build_names();
    }

    bool begin_panel(bool& opened, bool& focused, bool save_pos = true, bool always_active = false)
    {
        // m_class.ClassId = cast::to_u32(data()->class_id);
        // ImGui::SetNextWindowClass(&m_class);

        const std::string* name = &m_name_full;
        if (m_panel_w > 0 && m_panel_w < m_panel_name_w + 60.0f)
            name = &m_name_shortened_name_full;

        m_always_active = always_active;
        if (m_always_active)
        {
            ImGui::PushStyleColor(ImGuiCol_TabHovered, data()->clr_active_hov_wf);
            ImGui::PushStyleColor(ImGuiCol_TabActive, data()->clr_active_wf);
            ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, data()->clr_active_wf);
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, data()->clr_active_wf);
            ImGui::PushStyleColor(ImGuiCol_TitleBg, data()->clr_active_wf);
        }

        if (m_activate)
        {
            m_activate = false;
            ImGui::SetWindowFocus(name->c_str());
        }

        opened = true;
        focused = false;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, data()->clr_bg);
        const bool expanded = ImGui::Begin(name->c_str(), &opened, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
            (save_pos ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoSavedSettings));
        m_panel_w = ImGui::GetWindowWidth();

        if (expanded && m_need_focus)
            focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        return expanded;
    }

    inline DlgCode end_panel(bool opened)
    {
        ImGui::End();
        ImGui::PopStyleColor(m_always_active ? 6 : 1);
        return opened ? DlgCode::Progress : DlgCode::Cancel;
    }

    inline void init() override
    {
        BaseDlgWithData<DataType>::init();
        data()->clr_bg = g::pOptsManager->color_load(AppColor::PanelBg);
        data()->clr_active_wf = g::pOptsManager->color_load(AppColor::PanelActiveWfTab);
        data()->clr_active_hov_wf = g::pOptsManager->color_load(AppColor::PanelActiveHoveredWfTab);
    }

private:
    inline void build_names()
    {
        m_name_full = fmt::format("{} {}{}", m_icon, m_name, m_id);
        m_name_shortened_name_full = fmt::format("{}{}", m_icon, m_id);
        m_panel_name_w = ImGui::CalcTextSize(fmt::format("{} {}", m_icon, m_name).c_str()).x;
    }

private:
    std::string m_id;
    std::string m_name;
    std::string m_icon;
    std::string m_name_full;
    std::string m_name_shortened_name_full;
    float m_panel_name_w = 0.0f;
    float m_panel_w = 0.0f;
    bool m_need_focus = false;
    bool m_always_active = false;
    bool m_activate = false;
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
        std::function<void(int32_t)> on_alloc_tt_data = nullptr;
        std::function<void(int32_t)> on_release_tt_data = nullptr;
        std::function<void(int32_t)> on_show_tt = nullptr;
        std::function<bool(int32_t)> on_pre_tt = nullptr;
        std::function<void(int32_t)> on_post_tt = nullptr;
    };

    struct DndCallbacks
    {
        std::function<bool(int32_t)> on_dnd_allow = nullptr;
        std::function<void(int32_t)> on_dnd_show = nullptr;
        const char* dnd_id = nullptr;
    };

    // returns true if dirty
    // draw_boudnded_rect applicable only for expandable items
    bool draw_list(std::vector<PanelListItem>& items, PanelItemState& dirty_item, 
        const TtCallbacks* tt_callbacks = nullptr, const DndCallbacks* dnd_callbacks = nullptr);

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

    struct Element
    {
        bool reset_state = false;   // immediately stop drawing

        enum class Type { Empty, Button, Input, InputHint, Text };
        Type type = Type::Button;

        bool enabled = true;

        std::function<void(Element&)> on_click = nullptr;
        std::function<void(Element&)> on_preview = nullptr;
        std::function<void(Element&)> on_postview = nullptr;

        // button
        const char* icon = nullptr;
        const char* tooltip = nullptr;
        bool toggler = false;
        bool toggled = false;

        // input
        std::string* buffer = nullptr;
        const char* hint = nullptr;
        const char* id = nullptr;
        float input_w = 0.5f; // in percents of total width

        // text
        std::string text;
        float text_w = 0.0f;
        ImU32 text_clr = 0;
        bool text_bold = false;
    };

    void clear_toolbar();
    void add_toolbar_element(Element&& el, bool left_aligned);
    void finalize_toolbar();
    inline void set_use_regular_size_icons() { m_use_big_icons = false; }

private:
    std::vector<Element> m_elements_l;
    std::vector<Element> m_elements_r;
    ImU32 m_clr_hover = 0;
    ImU32 m_clr_pressed = 0;
    bool m_use_big_icons = true;
};


} // namespace ui
