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
    void begin_group(OptsData& data, float padding, float& avail_x, float& item_w, float& offset);
    void end_group(float padding);

    void show_connection_opts(OptsData& data, const char*& help_text, float padding, float avail_x, float item_w, float offset);
    void show_interaction_opts(OptsData& data, const char*& help_text, float padding, float avail_x, float item_w, float offset);

private:
    bool m_data_changed = false;
};


}
