/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelServerLogs final
    : public PanelEmptyWithDescr
    , public PanelToolbar
    , public PanelBase<PanelCommonData>
{
public:
    PanelServerLogs();
    ~PanelServerLogs() noexcept override;

    void init() override;

    void get_parsed_address(std::string& host, int64_t& port) const { host = m_host; port = m_port; }

private:
    DlgCode show(bool& focused) override;
    void show_logs();

    bool update_logs();
    void search_for_address(size_t from, size_t to);

    void create_toolbar();
    void on_search_up(PanelToolbar::Element& el);
    void on_search_down(PanelToolbar::Element& el);

private:
    std::deque<std::string> m_logs;    
    bool m_autoscroll = true;
    ImU32 m_clr_hover = 0;
    ImU32 m_clr_pressed = 0;
    std::string m_search;
    int32_t m_found_index = -1;
    bool m_scroll_to_found = false;
    std::string m_host;
    int64_t m_port = 0;
};


} // namespace ui
