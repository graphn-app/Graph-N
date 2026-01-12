
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

    void th_api();
    void th_comfyui();
    void th_ws();

    void calc_fps();
    void init_disks();

    struct MetricLimits;
    ImU32 get_color(int64_t value, bool& stay_default, const MetricLimits& lim);

    void show_config_menu();

private:
    std::atomic_bool m_stop = false;
    std::atomic_bool m_pause = false;

    std::thread m_th_api;
    std::thread m_th_comfyui;
    std::thread m_th_ws;

    struct VideoDevice final
    {
        std::string name;
        uint64_t vram = 0;
        uint64_t pytorch_total = 0;
        uint64_t pytorch_free = 0;
    };

    struct Queue final
    {
        uint64_t node_id = 0;
        uint32_t step = 0;
        uint32_t total = 0;
    };

    struct Disks final
    {
        bool enabled = true;
        std::string path;
        std::string format_path;
        uint64_t free = 0;
    };

    struct MetricLimits final
    {
        int64_t warning = 0;
        int64_t critical = 0;
        bool is_lower_better = false; // true for ping, false for others
    };

    std::mutex m_lock;
    std::list<VideoDevice> m_video_devices;     // comfyui api /system_stats
    Queue m_queue;                              // ws
    std::vector<Disks> m_disks;                 // os api
    std::atomic_uint64_t m_lram = 0;            // os api
    std::atomic_uint64_t m_sram = 0;            // comfyui api /system_stats
    std::atomic_uint32_t m_queue_remaining = 0; // comfyui api /prompt
    uint64_t m_fps = 0;                         // imgui
    std::atomic_uint64_t m_ping = 0;            // ws

    MetricLimits m_lim_lram;
    MetricLimits m_lim_sram;
    MetricLimits m_lim_vram;    
    MetricLimits m_lim_pytorch;
    MetricLimits m_lim_disk;
    MetricLimits m_lim_ping;
    MetricLimits m_lim_fps;

    fmt::memory_buffer m_buffer;
};


} // namespace ui
