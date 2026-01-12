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

private:
    struct MetricLimits final
    {
        int64_t warning = 0;
        int64_t critical = 0;
    };

    MetricLimits m_lim_lram;
    MetricLimits m_lim_sram;
    MetricLimits m_lim_vram;
    MetricLimits m_lim_pytorch;
    MetricLimits m_lim_disk;
    MetricLimits m_lim_ping;
    MetricLimits m_lim_fps;

    std::vector<std::pair<std::string, bool>> m_disks;
};


}
