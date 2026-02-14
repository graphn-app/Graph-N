
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelServerLogs final : 
    public PanelEmptyWithDescr,
    public PanelToolbar,
    public PanelBase<PanelCommonData>
{
public:
    PanelServerLogs();
    ~PanelServerLogs() override;

    void init() override;
    void stop_all_threads() override;

private:
    DlgCode show(bool& focused) override;
    void show_logs();

    void create_toolbar();

    void on_pre_search(PanelToolbar::Element& el);
    void on_search_up(PanelToolbar::Element& el);
    void on_search_down(PanelToolbar::Element& el);
    void on_autoscroll(PanelToolbar::Element& el);
    void on_clear(PanelToolbar::Element& el);

private:
    std::list<std::string> m_logs;
    std::string m_search;
    bool m_autoscroll = true;
    ImU32 m_clr_hover = 0;
    ImU32 m_clr_pressed = 0;
};


} // namespace ui
