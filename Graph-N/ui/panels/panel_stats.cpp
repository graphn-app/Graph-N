
#include "ui/panels/panel_stats.h"
#include "core/managers/settings_manager.h"

namespace ui
{

PanelStats::PanelStats()
    : PanelBase(VisibleObject::PanelStats, "Stats", "DFBE846D7B1C49BF8D70C2AF99ADCBEF")
{}

PanelStats::~PanelStats() = default;

void PanelStats::init()
{
    LogF;
    PanelBase::init();

    // init disks, read settings

    m_stop = false;
    m_th_disks = std::thread(&PanelStats::th_disks, this);
    m_th_comfyui = std::thread(&PanelStats::th_comfyui, this);
    m_th_ws = std::thread(&PanelStats::th_ws, this);
}

void PanelStats::stop_all_threads()
{
    m_stop = true;
    if (m_th_disks.joinable())  m_th_disks.join();
    if (m_th_comfyui.joinable())  m_th_comfyui.join();
    if (m_th_ws.joinable())  m_th_ws.join();
}

void PanelStats::th_disks()
{
    using namespace std::chrono_literals;

    const auto check_interval = 5000ms;
    const auto sleep_step = 150ms;
    auto accumulated_time = 0ms;

    while (!m_stop)
    {
        std::this_thread::sleep_for(sleep_step);
        if (m_stop) break;

        accumulated_time += sleep_step;
        if (accumulated_time >= check_interval)
        {
            // Do calculations
            // ...

            accumulated_time = 0ms;
        }
    }
}

void PanelStats::th_comfyui()
{
    using namespace std::chrono_literals;

    const auto check_interval = 2000ms;
    const auto sleep_step = 150ms;
    auto accumulated_time = 0ms;

    while (!m_stop)
    {
        std::this_thread::sleep_for(sleep_step);
        if (m_stop) break;

        accumulated_time += sleep_step;
        if (accumulated_time >= check_interval)
        {
            // Do calculations
            // ...

            accumulated_time = 0ms;
        }
    }
}

void PanelStats::th_ws()
{
    using namespace std::chrono_literals;

    const auto check_interval_idle = 2000ms;
    const auto check_interval_executing = 1000ms;
    const auto sleep_step = 150ms;
    auto accumulated_time = 0ms;

    while (!m_stop)
    {
        std::this_thread::sleep_for(sleep_step);
        if (m_stop) break;

        accumulated_time += sleep_step;
        if (accumulated_time >= check_interval)
        {
            // Do calculations
            // ...

            accumulated_time = 0ms;
        }
    }
}

void PanelStats::calc_fps()
{}

DlgCode PanelStats::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
    {
        calc_fps();
    }
    return end_panel(opened);
}

} // namespace ui
