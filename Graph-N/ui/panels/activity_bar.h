#pragma once
#include "ui/base_dlg.h"

namespace ui
{

struct FontGuard;

class ActivityBar final
{
public:
    ActivityBar();
    ~ActivityBar();

    void register_panels(const std::vector<DlgObj>& panels);
    void show(ActivityBarMode mode);

    inline float get_l_width() const { return m_bar_l_width; }
    inline float get_r_width() const { return m_bar_r_width; }

private:
    void calc_window_sizes(bool left, bool right);
    void recalc_viewport_size(bool left, bool right);

    enum class Direction { None, Left, Right, Bottom };
    Direction get_panel_direction(DlgObj panel_obj, ImRect center_rc);
    void draw_panel_buttons(DlgObj panel_obj, bool active);
    void draw_additional_buttons();

    void show_tooltip(FontGuard* fg, const char* text);

private:
    ImU32 m_clr_bg = 0;
    ImU32 m_clr_inactive = 0;

    ImU32 m_clr_pressed = 0;
    ImU32 m_clr_hover = 0;
    ImU32 m_clr_active = 0;

    ImU32 m_clr_restart_inactive = 0;
    ImU32 m_clr_restart_pressed = 0;
    ImU32 m_clr_restart_hover = 0;

    std::vector<std::pair<DlgObj, Direction>> m_panels;
    ActivityBarMode m_mode;
    ImRect m_left_rc;
    ImRect m_right_rc;

    float m_bar_l_width = 0.0f;
    float m_bar_r_width = 0.0f;
};

} // namespace ui
