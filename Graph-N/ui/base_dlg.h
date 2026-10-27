/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/managers/settings_manager.h"

// TODO - remove commented
// check if we need DlgData as template

namespace ui
{

struct DlgData
{
    NO_COPY_MOVE_VCTORS(DlgData);

    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;

    ImU32 clr_bg = 0;
    ImU32 clr_title = 0;

    bool open_popup = true;
    bool set_focus = true;

    std::vector<std::string> buffer;
};

class BaseDlg
{
public:
    BaseDlg(DlgObj obj) noexcept
        : m_obj(obj)
    {}
    virtual ~BaseDlg() noexcept = default;

    NO_COPY_MOVE(BaseDlg);

    DlgCode show_dialog(bool& focused)
    {
        if (m_close_called) [[unlikely]]
            return DlgCode::Cancel;
        const auto ret = show(focused);
        m_appeared = false;
        return ret;
    }
    void close_dialog() noexcept
    {
        m_close_called = true;
    }
    void on_appear() noexcept 
    {
        m_appeared = true;
    }

    virtual void init() {}
    virtual void stop_and_cleanup() {}

protected:
    virtual DlgCode show(bool& focused) = 0;

    //void update_window_pos()
    //{
    //	const auto pos = ImGui::GetWindowPos();
    //	m_data->x = pos.x;
    //	m_data->y = pos.y;
    //}

    DlgObj get_dlg_obj() const noexcept { return m_obj; }

    //void set_exit_code(DlgCode code) { m_exit_code = code; }
    //void set_exit_code_ok() { m_exit_code = DlgCode::OK; }
    //DlgCode exit_code() const { return m_exit_code; }

    DlgCode cleanup_and_exit(bool in_progress, bool end_dlg = true, bool pop_title_styles = true, bool pop_bg_style = true) noexcept
    {
        if (end_dlg)
            ImGui::End();

        if (pop_bg_style)
            ImGui::PopStyleColor();

        if (pop_title_styles)
            ImGuiEx::PopDlgTitleStyles();

        return in_progress ? DlgCode::Progress : DlgCode::Cancel;
    }

protected:
    bool m_appeared = false;

private:
    DlgObj m_obj;
    DlgCode m_exit_code = DlgCode::Progress;
    bool m_close_called = false;
};

template <typename DlgDataType>
class BaseDlgWithData : public BaseDlg
{
public:
    using BaseDlg::BaseDlg;
    ~BaseDlgWithData() noexcept override = default;

    void init() override
    {
        if (m_dlg_initialized)
            return;
        m_data = std::make_unique<DlgDataType>();
        dlg_data()->clr_bg = g::pOptsManager->color_load(AppColor::PanelBg);
        m_dlg_initialized = true;
    }

protected:
    DlgDataType* dlg_data() const noexcept { return m_data.get(); }

private:
    bool m_dlg_initialized = false;
    std::unique_ptr<DlgDataType> m_data;
};

} // namespace ui
