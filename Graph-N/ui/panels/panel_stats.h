
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

class PanelStats final : public PanelBase<PanelCommonData>
{
public:
    PanelStats();
    ~PanelStats() override;

    void init() override;
    void stop_all_threads() override;

private:
    DlgCode show(bool& focused) override;

    void th_disks();
    void th_comfyui();
    void th_ws();
    void calc_fps();

private:
    std::atomic_bool m_stop = false;
    std::thread m_th_disks;
    std::thread m_th_comfyui;
    std::thread m_th_ws;

    struct VideoDevice
    {
        std::string name;
        uint64_t vram = 0;
    };

    struct Queue
    {
        uint64_t node_id = 0;
        uint32_t step = 0;
        uint32_t total = 0;
    };

    struct Disks
    {
        bool enabled = true;
        std::string path;
        uint64_t space = 0;
    };

    std::mutex m_mutex;
    std::list<VideoDevice> m_video_devices;
    Queue m_queue;
    Disks m_disks;

    std::atomic_ullong m_ram = 0;
    uint32_t m_fps = 0;
    std::atomic_uint32_t m_ping = 0;
    std::atomic_bool m_executing = false;
};


} // namespace ui
