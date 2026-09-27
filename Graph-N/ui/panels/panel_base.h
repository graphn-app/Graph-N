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
    PanelBase(DlgObj obj, const std::string& name, const std::string& id = "")
        : BaseDlgWithData<DataType>(obj)
    {
        m_id = id.empty() ? DlgObj_SID[cast::to_szt(obj)] : id;
        if (m_id.find("###") == std::string::npos)
            m_id = "###" + m_id;
        m_icon = g::pFlow->get_panel_icon(obj);
        m_name = name;
        build_names();
        Log("Created panel: " << m_name_full << ", id: " << m_id);
    }

    ~PanelBase() noexcept override
    {
        Log("Destroyed panel: " << m_name_full << ", id: " << m_id);
    }

    const char* get_panel_name() const  noexcept{ return m_name.c_str(); }
    const char* get_panel_name_full() const noexcept { return m_name_full.c_str(); }
    const char* get_panel_id() const noexcept { return m_id.c_str(); }

    void set_force_focused(bool focused) noexcept { m_grab_focus = focused; }
    bool get_force_focused() const noexcept { return m_grab_focus; }

    void mark_dirty(bool dirty)
    {
        if (dirty == m_dirty)
            return;
        if (m_dirty)
            m_name.pop_back();
        else
            m_name.push_back('*');
        m_dirty = dirty;
        build_names();
        Log("Panel: " << m_id << " changed status to: " << (dirty ? "dirty" : "clean"));
    }

    void set_sequential_number(size_t number) { m_sequential_number = number; build_names(); }

    void init() override
    {
        BaseDlgWithData<DataType>::init();
        dlg_data()->clr_bg = g::pOptsManager->color_load(AppColor::PanelBg);
        dlg_data()->clr_active_wf = g::pOptsManager->color_load(AppColor::PanelActiveWfTab);
        dlg_data()->clr_active_hov_wf = g::pOptsManager->color_load(AppColor::PanelActiveHoveredWfTab);
    }

protected:
    using BaseDlgWithData<DataType>::dlg_data;

    void set_name(const std::string& new_name)
    {
        m_name = new_name;
        build_names();
        Log("Panel id " << m_id << " renamed to " << m_name_full);
    }
    void set_id(const std::string& new_id)
    {
        m_id = new_id;
        if (m_id.find("###") == std::string::npos)
            m_id = "###" + m_id;
        build_names();
        Log("Panel id " << m_id << " renamed to " << m_name_full);
    }

    bool begin_panel(bool& opened, bool& focused, bool save_pos = true) noexcept
    {
        // m_class.ClassId = cast::to_u32(dlg_data()->class_id);
        // ImGui::SetNextWindowClass(&m_class);

        const std::string* name = &m_name_full;
        if (m_panel_w > 0 && m_panel_w < m_panel_name_w + 60.0f)
            name = &m_name_shortened_name_full;

        if (m_grab_focus)
        {
            m_grab_focus = false;
            ImGui::SetWindowFocus(name->c_str());
        }

        opened = true;
        focused = false;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, dlg_data()->clr_bg);
        const bool expanded = ImGui::Begin(name->c_str(), &opened, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
            (save_pos ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoSavedSettings));
        m_panel_w = ImGui::GetWindowWidth();

        if (expanded)
            focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        return expanded;
    }

    DlgCode end_panel(bool opened) noexcept
    {
        ImGui::End();
        ImGui::PopStyleColor();
        return opened ? DlgCode::Progress : DlgCode::Cancel;
    }

private:
    void build_names()
    {
        if (m_sequential_number > 0 && m_sequential_number < 10)
            m_name_full = fmt::format(FMT_STRING("{} [{}]: {}{}"), m_icon, m_sequential_number, m_name, m_id);
        else
            m_name_full = fmt::format(FMT_STRING("{} {}{}"), m_icon, m_name, m_id);
        m_name_shortened_name_full = fmt::format(FMT_STRING("{}{}"), m_icon, m_id);
        m_panel_name_w = ImGui::CalcTextSize(fmt::format(FMT_STRING("{} {}"), m_icon, m_name).c_str()).x;
    }

private:
    std::string m_id;
    std::string m_name;
    std::string m_icon;
    std::string m_name_full;
    std::string m_name_shortened_name_full;
    size_t m_sequential_number = 0;
    float m_panel_name_w = 0.0f;
    float m_panel_w = 0.0f;
    bool m_grab_focus = false;
    bool m_dirty = false;
    //  ImGuiWindowClass m_class;
};

class PanelEmptyWithDescr
{
public:
    PanelEmptyWithDescr() noexcept = default;
    ~PanelEmptyWithDescr() noexcept = default;

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
    struct TtCallbacks 
    {
        std::function<void(int32_t)> on_alloc_tt_data;      // allocate tooltip data
        std::function<void(int32_t)> on_release_tt_data;    // release tooltiop data
        std::function<bool(int32_t)> on_pre_tt;             // prepare before tooltip
        std::function<void(int32_t)> on_show_tt;            // show tooltip
        std::function<void(int32_t)> on_post_tt;            // cleanup after tooltip
    };

    struct DndCallbacks 
    {
        const char* dnd_id = nullptr;                       // drag&drop identifier
        std::function<bool(int32_t)> on_dnd_allow;          // do need to start drag&drop
        std::function<void(int32_t)> on_dnd_show;           // process drag&drop
    };

    bool draw_list(const std::vector<PanelListItem>& items, PanelItemEvent& out_event, 
        const TtCallbacks* tt_callbacks = nullptr, const DndCallbacks* dnd_callbacks = nullptr);
    void begin_rename(int32_t index, const std::string& current_name);

protected:
    void go_tooltip(int32_t tt_index, const TtCallbacks* tt_callbacks);
    bool release_tooltip(int32_t tt_index, const TtCallbacks* tt_callbacks);

private:
    int32_t m_hovered_item = -1;
    int32_t m_tt_index = -1;
    float m_tt_life_timer = 0.0f;

    int32_t m_renaming_idx = -1;
    std::string m_rename_buf;
    bool m_need_rename_focus = false;
};

class PanelToolbar
{
public:
    PanelToolbar() noexcept = default;
    ~PanelToolbar() noexcept = default;

    NO_COPY_MOVE(PanelToolbar);

    void draw_toolbar();

    struct Element final
    {
        enum class Type : uint8_t { Empty, Button, Input, InputHint, Text, InputInt, Combo };
        Type type = Type::Button;

        bool enabled = true;
        bool blinking = false; // only for button and text

        std::function<void(Element&)> on_click = nullptr;
        std::function<void(Element&)> on_preview = nullptr;
        std::function<void(Element&)> on_postview = nullptr;

        // button
        const char* icon = nullptr;

        const char* tooltip = nullptr;
        std::string tooltip_hotkey;
        bool tooltip_next_line = false;
        const char* tooltip_hotkey_separator = " | ";

        bool toggler = false;
        bool toggled = false;

        // input
        std::string* buffer_str = nullptr; // input text
        int64_t* buffer_int = nullptr; // input int
        const char* hint = nullptr; // input text only
        const char* id = nullptr;
        float input_w = 0.5f; // input text: in percents of total width, input int: in pixels
        int64_t int_min = 0; // input int only
        int64_t int_max = 0; // input int only

        // text
        const char* text = nullptr;
        const char* text_end = nullptr;     // internal. do not use
        ImU32 text_clr = 0;
        bool text_bold = false;
        float text_width = 0.0f;

        // combo
        std::vector<const char*> cb_items;
        int32_t cb_item_sel = 0;
    };

    void clear_toolbar() noexcept;
    void add_toolbar_element(Element&& el, bool left_aligned);
    void finalize_toolbar(bool add_separator) noexcept;
    void set_use_regular_size_icons() noexcept { m_use_big_icons = false; }
    void reset_toolbar_state() noexcept { m_reset_state = true; }

private:
    void draw_background();

private:
    std::vector<Element> m_elements_l;
    std::vector<Element> m_elements_r;
    ImU32 m_clr_hover = 0;
    ImU32 m_clr_pressed = 0;
    bool m_use_big_icons = false;
    bool m_draw_separator = false;
    bool m_reset_state = false;
};


} // namespace ui
