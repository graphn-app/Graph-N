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

private:
    void show_connection_opts(OptsData& data, const char*& help_text);
    void show_interaction_opts(OptsData& data, const char*& help_text);

private:
    bool m_data_changed = false;
};


}
