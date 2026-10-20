/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "ui/base_dlg.h"
#include "ui/dialogs/options/opts_defs.h"

namespace ui
{

class OptRightPane;

class OptionsDlgs final : public BaseDlgWithData<DlgData>
{
public:
    OptionsDlgs() noexcept;
    ~OptionsDlgs() noexcept override;

    void init() override;
    void select_right_pane(OptPane pane) noexcept;

private:
    DlgCode show(bool& focused) override;
    void show_left_pane();
    void show_right_pane();

private:
    OptsData m_opts_data;
    std::unique_ptr<OptRightPane> m_right_pane;
};


}
