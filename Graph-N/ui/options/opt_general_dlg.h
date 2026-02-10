#pragma once
#include "ui/options/opts_defs.h"
#include "ui/options/opts_right_pane.h"

namespace ui
{

class OptGeneralDlg final : public OptRightPane
{
public:
    OptGeneralDlg() = default;
    ~OptGeneralDlg() override;

    void init(OptsData& data) override;
    void show(OptsData& data) override;
};

// показывать сетку

}
