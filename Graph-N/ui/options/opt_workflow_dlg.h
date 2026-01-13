#pragma once
#include "ui/options/opts_defs.h"
#include "ui/options/opts_right_pane.h"

namespace ui
{

class OptWorkflowDlg final : public OptRightPane
{
public:
    OptWorkflowDlg() = default;
    ~OptWorkflowDlg() override;

    void init(OptsData& data) override;
    void show(OptsData& data) override;

private:
    void show_input_styles(OptsData& data);
};

// показывать сетку

}
