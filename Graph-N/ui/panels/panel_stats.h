/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "ui/panels/panel_base.h"
#include "core/listeners.h"

namespace ui
{

class PanelStats final
    : public PanelBase<PanelCommonData>
    , public listener::Timer
{
public:
    PanelStats();
    ~PanelStats() noexcept override;

    void init() override;
    void stop_and_cleanup() override;

private:
    struct ServerStats;

    DlgCode show(bool& focused) override;
    void show_this_pc_as_text(float val_offset);
    void show_server_as_text(float val_offset, const ServerStats* stats);
    void show_this_pc_as_bar(float val_offset);
    void show_server_as_bar(float val_offset, const ServerStats* stats);
    void show_app_performance(float val_offset);

    void th_api();
    void th_comfyui();

    void print_fps(float val_offset);
    void print_ping(float val_offset) noexcept;
    void print_text_stat(const char* stat_name, float val_offset, uint64_t value, int64_t lim_warn, int64_t lim_crit, bool is_lower_better, bool do_format_bytes);
    void print_bar_stat(const char* stat_name, float val_offset, uint64_t free, uint64_t total, int64_t lim_warn, int64_t lim_crit, const char* tooltip);
    void init_disks();

    void format_bytes(uint64_t bytes);
    ImU32 get_color(int64_t value, bool& stay_default, int64_t lim_warn, int64_t lim_crit, bool is_lower_better) noexcept;

    // Lazy1sTimer
    void on_1s_lazy_timer(int64_t total_seconds_elapsed) noexcept override;

private:
    std::atomic_bool m_pause = false;

    std::thread m_th_api;
    std::thread m_th_comfyui;

    struct VideoDevice final
    {
        std::string name;
        uint64_t vram_total = 0;
        uint64_t vram_free = 0;
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
        const std::pair<std::string, bool>* disk_opt = nullptr;
        std::string format_path;
        std::atomic_uint64_t free = 0;
        std::atomic_uint64_t total = 0;

        Disks() = default;
        Disks(Disks&& other) noexcept
            : disk_opt(other.disk_opt)
            , format_path(std::move(other.format_path))
            , free(other.free.load(std::memory_order_relaxed))
            , total(other.total.load(std::memory_order_relaxed))
        {}
    };

    struct ServerStats final    
    {
        std::string comfy_ver;
        std::list<VideoDevice> video_devices;
        uint64_t sram_free = 0;
        uint64_t sram_total = 0;
    };

    std::vector<Disks> m_disks;                                     // os api
    std::atomic<std::shared_ptr<const ServerStats>> m_server_stats; // comfyui api /system_stats 
    std::atomic_uint64_t m_lram_free = 0;                           // os api
    std::atomic_uint64_t m_lram_total = 0;                          // os api
    std::atomic_uint32_t m_queue_remaining = 0;                     // comfyui api /prompt
    uint64_t m_fps = 0;                                             // imgui
    int64_t m_ping_mk = 0;


    fmt::memory_buffer m_buffer;
};


} // namespace ui
