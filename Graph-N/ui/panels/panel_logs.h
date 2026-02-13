
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelServerLogs final : 
    public PanelEmptyWithDescr,
    public PanelBase<PanelCommonData>
{
public:
    PanelServerLogs();
    ~PanelServerLogs() override;

    void init() override;
    void stop_all_threads() override;

private:
    DlgCode show(bool& focused) override;
    void show_toolbar();
    void show_logs();

private:
    std::list<std::string> m_logs;
};


} // namespace ui
