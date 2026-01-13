#pragma once
#include "ui/options/opts_defs.h"
#include "ui/options/opts_right_pane.h"

namespace ui
{

class OptStatsDlg final : public OptRightPane
{
public:
    OptStatsDlg() = default;
    ~OptStatsDlg() override = default;

    void init(OptsData& data) override;
    void show(OptsData& data) override;
};


}
