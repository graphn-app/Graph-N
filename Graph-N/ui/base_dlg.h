#pragma once

// TODO - remove commented
// check if we need DlgData as template

namespace ui
{

struct DlgData
{
    DlgData() = default;
    virtual ~DlgData() = default;

    NO_COPY_MOVE(DlgData);

    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;

    bool open_popup = true;
    bool set_focus = true;

    std::vector<std::string> buffer;

    ImU32 clr_bg = 0;
    ImU32 clr_title = 0;

    //gid set_focus_id = 0;

    //std::map<gid, std::string> buffer;
    //std::map<gid, tools::dt> date_buffer;
};

class BaseDlg
{
public:
    BaseDlg(VisibleObject obj)
        : m_obj(obj)
    {}
    virtual ~BaseDlg() = 0;

    NO_COPY_MOVE(BaseDlg);

    void set_focused() { m_focused = true; }

    DlgCode show_dialog(bool& focused)
    {
        if (m_close_called)
            return DlgCode::cancel;
        return show(focused);
    }
    void close_dialog()
    {
        m_close_called = true;
    }

    virtual void stop_all_threads() {}

protected:
    virtual DlgCode show(bool& focused) = 0;

    //void update_window_pos()
    //{
    //	const auto pos = ImGui::GetWindowPos();
    //	m_data->x = pos.x;
    //	m_data->y = pos.y;
    //}

    VisibleObject get_dlg_obj() const { return m_obj; }

    //void set_exit_code(DlgCode code) { m_exit_code = code; }
    //void set_exit_code_ok() { m_exit_code = DlgCode::ok; }
    //DlgCode exit_code() const { return m_exit_code; }

    DlgCode cleanup_and_exit(bool in_progress, bool end_dlg = true, bool pop_title_styles = true, bool pop_bg_style = true)
    {
        if (end_dlg)
            ImGui::End();

        if (pop_bg_style)
            ImGui::PopStyleColor();

        if (pop_title_styles)
            ImGui::PopDlgTitleStyles();

        return in_progress ? DlgCode::progress : DlgCode::cancel;
    }

    DlgCode cleanup_and_exit_popup(bool end_dlg, bool pop_styles, DlgCode exit_code)
    {
        if (end_dlg)
            ImGui::EndPopup();

        if (pop_styles)
            ImGui::PopDlgTitleStyles();

        return exit_code;
    }

    void set_focus()
    {
        if (m_focused)
            ImGui::SetNextWindowFocus();
        m_focused = false;
    }

private:
    VisibleObject m_obj;
    DlgCode m_exit_code = DlgCode::progress;
    bool m_focused = false;
    bool m_close_called = false;
};

inline BaseDlg::~BaseDlg() = default;

template <typename DlgDataType>
class BaseDlgWithData : public BaseDlg
{
public:
    using BaseDlg::BaseDlg;
    ~BaseDlgWithData() override = default;

    virtual void init()
    {
        if (m_dlg_initialized)
            return;
        m_data = std::make_unique<DlgDataType>();
        m_dlg_initialized = true;
    }

protected:
    DlgDataType* data() { return m_data.get(); }

private:
    bool m_dlg_initialized = false;
    std::unique_ptr<DlgDataType> m_data;
};

} // namespace ui
