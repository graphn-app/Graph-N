#pragma once
#include "ui/options/opts_defs.h"
#include "ui/options/opts_right_pane.h"

namespace ui
{

class OptComfyDlg final : public OptRightPane
{
public:
    OptComfyDlg() = default;
    ~OptComfyDlg() override;

    void init(OptsData& data) override;
    void show(OptsData& data) override;
};


}
