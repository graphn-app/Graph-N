#pragma once
#include "ui/base_dlg.h"
#include "ui/options/opts_defs.h"

namespace ui
{

class OptRightPane;

class OptionsDlgs final : public BaseDlgWithData<DlgData>
{
public:
    OptionsDlgs(OptPane init_opt = OptPane::_none);
    ~OptionsDlgs() override;

    void init() override;
    void select_right_pane(OptPane pane);

private:
    DlgCode show(bool& focused) override;
    void show_left_pane();
    void show_right_pane();

private:
    OptsData m_opts_data;
    std::map<OptPane, std::unique_ptr<OptRightPane>> m_right_panes;
};


}
