/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/panels/panel_stats.h"
#include "ui/graph-n_flow.h"
#include "core/managers/connect_manager.h"
#include "core/base/comfyui_web_paths.h"
#include "main.h"

namespace ui
{

PanelStats::PanelStats()
    : PanelBase(DlgObj::PanelStats, "Stats")
{}

PanelStats::~PanelStats() noexcept
{
    if (g::pGraphN != nullptr)
        g::pGraphN->unsubscribe_timers(this);
}

void PanelStats::init()
{
    PanelBase::init();

    init_disks();

    m_pause.store(false, std::memory_order_relaxed);
    m_th_api = std::thread(&PanelStats::th_api, this);
    m_th_comfyui = std::thread(&PanelStats::th_comfyui, this);

    g::pGraphN->subscribe_timers(this);
}

void PanelStats::init_disks()
{
    m_disks.clear();
    const auto& enabled_disks = g::pOptsManager->stat_disks_enabled_load();
    for (auto& opt : enabled_disks)
    {
        Disks d;
        d.disk_opt = &opt;
#ifdef _WIN32
        d.format_path = fmt::format(FMT_STRING("Disk {}"), d.disk_opt->first);
#else
        d.format_path = d.path;
#endif
        m_disks.push_back(std::move(d));
    }
}

void PanelStats::stop_and_cleanup()
{
    m_pause = false;
    if (m_th_api.joinable())  m_th_api.join();
    if (m_th_comfyui.joinable())  m_th_comfyui.join();
}

void PanelStats::th_api()
{
    const auto check_ram_interval = 1000ms;
    const auto check_disk_interval = 5000ms;
    const auto sleep_step = 150ms;
    auto accumulated_time_disk = 0ms;
    auto accumulated_time_ram = 0ms;

    while (!g::app_global_stop_all_threads.stop_requested())
    {
        std::this_thread::sleep_for(sleep_step);
        if (g::app_global_stop_all_threads.stop_requested())
            break;
        if (m_pause.load(std::memory_order_relaxed))
        {
            accumulated_time_disk = check_disk_interval;
            accumulated_time_ram = check_ram_interval;
            continue;
        }

        accumulated_time_disk += sleep_step;
        accumulated_time_ram += sleep_step;

        if (accumulated_time_ram >= check_ram_interval)
        {
#ifdef _WIN32
            MEMORYSTATUSEX info;
            info.dwLength = sizeof(MEMORYSTATUSEX);
            if (::GlobalMemoryStatusEx(&info))
            {
                m_lram_free.store(info.ullAvailPhys, std::memory_order_relaxed);
                m_lram_total.store(info.ullTotalPhys, std::memory_order_relaxed);
            }
#elif defined __linux__
#error Not implemented
#elif defined __APPLE__
#error Not implemented
#endif

            accumulated_time_ram = 0ms;
        }
        if (accumulated_time_disk >= check_disk_interval)
        {
#ifdef _WIN32
            for (auto& disk : m_disks)
            {
                if (!disk.disk_opt->second)
                    continue;

                ULARGE_INTEGER free_bytes = {}, total_bytes = {}, total_free = {};
                if (::GetDiskFreeSpaceExA(disk.disk_opt->first.c_str(), &free_bytes, &total_bytes, &total_free))
                {
                    disk.free.store(free_bytes.QuadPart, std::memory_order_relaxed);
                    disk.total.store(total_bytes.QuadPart, std::memory_order_relaxed);
                }
            }
#elif defined __APPLE__
#error Not implemented
#elif defined __linux__
#error Not implemented
#endif

            accumulated_time_disk = 0ms;
        }
    }
}

void PanelStats::th_comfyui()
{
    const auto check_interval = 2500ms;
    const auto sleep_step = 150ms;
    auto accumulated_time = 0ms;

    bool system_stats_err = false;
    bool prompt_err = false;

    while (!g::app_global_stop_all_threads.stop_requested())
    {
        std::this_thread::sleep_for(sleep_step);
        if (g::app_global_stop_all_threads.stop_requested())
            break;
        if (m_pause.load(std::memory_order_relaxed) || !g::pFlow->get_online())
        {
            accumulated_time = check_interval;
            continue;
        }

        accumulated_time += sleep_step;
        if (accumulated_time >= check_interval)
        {
            std::string data;
            int32_t http_res = g::pFlow->http_get_and_wait(HttpRequestor::SystemStats_Stats, comfyui_http::system_stats, {}, {}, false,
                []() noexcept { return g::app_global_stop_all_threads.stop_requested(); }, data);

            if (HTTP_RESP_OK == http_res)
            {
                try
                {
                    auto stats = std::make_shared<ServerStats>();
                    const auto j = u_json::parse(data);
                    if (j.contains("system") && j["system"].contains("ram_free") && j["system"]["ram_free"].is_number_integer())
                    {
                        auto& jsystem = j["system"];
                        if (jsystem.contains("ram_free") && jsystem["ram_free"].is_number_integer())
                            stats->sram_free = jsystem["ram_free"].get<uint64_t>();
                        if (jsystem.contains("ram_total") && jsystem["ram_total"].is_number_integer())
                            stats->sram_total = jsystem["ram_total"].get<uint64_t>();
                    }
                    if (j.contains("system") && j["system"].contains("comfyui_version") && j["system"]["comfyui_version"].is_string())
                    {
                        auto& jsystem = j["system"];
                        stats->comfy_ver = jsystem["comfyui_version"].get<std::string>();
                    }
                    if (j.contains("devices") && j["devices"].is_array())
                    {
                        auto& jdevices = j["devices"];
                        for (auto& jdevice : jdevices)
                        {
                            VideoDevice vd;
                            if (jdevice.contains("name") && jdevice["name"].is_string())
                            {
                                vd.name = jdevice["name"];

                                tools::str::replace_this(vd.name, "cuda:0", "");
                                tools::str::replace_this(vd.name, "cuda:1", "");
                                tools::str::replace_this(vd.name, "cuda:2", "");
                                tools::str::replace_this(vd.name, "cuda:3", "");
                                tools::str::replace_this(vd.name, "cudaMallocAsync", "");
                                tools::str::replace_this(vd.name, "native_allocator", "");
                                tools::str::replace_this(vd.name, "NVIDIA GeForce", "");
                                tools::str::replace_this(vd.name, "NVIDIA", "");
                                tools::str::replace_this(vd.name, "AMD Radeon", "");
                                tools::str::replace_this(vd.name, "AMD", "");
                                tools::str::replace_this(vd.name, "Laptop GPU", "");
                                tools::str::trim_this(vd.name, " :");
                            }
                            if (jdevice.contains("vram_free") && jdevice["vram_free"].is_number_integer())
                            {
                                vd.vram_free = jdevice["vram_free"];
                            }
                            if (jdevice.contains("vram_total") && jdevice["vram_total"].is_number_integer())
                            {
                                vd.vram_total = jdevice["vram_total"];
                            }
                            if (jdevice.contains("torch_vram_total") && jdevice["torch_vram_total"].is_number_integer())
                            {
                                vd.pytorch_total = jdevice["torch_vram_total"];
                            }
                            if (jdevice.contains("torch_vram_free") && jdevice["torch_vram_free"].is_number_integer())
                            {
                                vd.pytorch_free = jdevice["torch_vram_free"];
                            }
                            stats->video_devices.push_back(std::move(vd));
                        }
                    }

                    m_server_stats.store(std::move(stats), std::memory_order_release);
                }
                catch (const std::exception& ex)
                {
                    if (!system_stats_err)
                        LogExcep("/system_stats parse error: " << ex.what());
                    system_stats_err = true;
                }
            }

            uint32_t queue_remaining = 0;

            http_res = g::pFlow->http_get_and_wait(HttpRequestor::Stats_Prompt, comfyui_http::prompt, {}, {}, false,
                []() noexcept { return g::app_global_stop_all_threads.stop_requested(); }, data);

            if (HTTP_RESP_OK == http_res)
            {
                try
                {
                    const auto j = u_json::parse(data);
                    if (j.contains("exec_info") && j["exec_info"].contains("queue_remaining") && j["exec_info"]["queue_remaining"].is_number_integer())
                    {
                        queue_remaining = j["exec_info"]["queue_remaining"].get<uint32_t>();
                    }
                }
                catch (const std::exception& ex)
                {
                    if (!prompt_err)
                        LogExcep("/prompt parse error: " << ex.what());
                    prompt_err = true;
                }
            }
            m_queue_remaining.store(queue_remaining, std::memory_order_relaxed);

            accumulated_time = 0ms;
        }
    }
}

void PanelStats::on_1s_lazy_timer(int64_t /*total_seconds_elapsed*/) noexcept
{
    g::pConnectManager->get_status(&m_ping_mk);
}

struct FrameSmoother final
{
    double values[60] = {};
    int32_t offset = 0;

    float update(double new_val) noexcept
    {
        values[offset] = new_val;
        offset = (offset + 1) % 60;

        double sum = 0;
        for (const double v : values)
        {
            sum += v;
        }
        return cast::to_float(sum / 60);
    }
};

ImU32 PanelStats::get_color(int64_t value, bool& stay_default, int64_t lim_warn, int64_t lim_crit, bool is_lower_better) noexcept
{
    stay_default = false;

    if (value > 0)
    {
        if (is_lower_better)
        {
            if (value >= lim_crit) return g::pOptsManager->color_load(AppColor::StatCrit);
            if (value >= lim_warn) return g::pOptsManager->color_load(AppColor::StatWarn);
        }
        else
        {
            if (value <= lim_crit) return g::pOptsManager->color_load(AppColor::StatCrit);
            if (value <= lim_warn) return g::pOptsManager->color_load(AppColor::StatWarn);
        }
    }

    stay_default = true;
    return g::pOptsManager->color_load(AppColor::StatNorm);
}

void PanelStats::format_bytes(uint64_t bytes)
{        
    static constexpr std::string_view suffixes[] = { "B", "KB", "MB", "GB", "TB", "PB" };
    size_t suffix_index = 0;
    double count = cast::to_double(bytes);

    while (count >= 1024.0 && suffix_index < 5)
    {
        count /= 1024.0;
        suffix_index++;
    }

    m_buffer.clear();
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{:.2f} {}"), count, suffixes[suffix_index]);
};

DlgCode PanelStats::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        m_pause.store(false, std::memory_order_relaxed);

        static constexpr const std::string_view this_pc = "This PC";
        static const float this_pc_wid = ImGui::CalcTextSize(this_pc.data(), this_pc.data() + this_pc.size()).x;

        static constexpr const std::string_view server = "Server";
        static const float server_wid = ImGui::CalcTextSize(server.data(), server.data() + server.size()).x;

        static constexpr const std::string_view queue = "Queue";
        static const float queue_wid = ImGui::CalcTextSize(queue.data(), queue.data() + queue.size()).x;

        static constexpr const std::string_view performance = APP_CAPTION;
        static const float performance_wid = ImGui::CalcTextSize(performance.data(), performance.data() + performance.size()).x;

        static const float val_text_offset = ImGui::CalcTextSize("PyTorch total").x + ImGui::GetStyle().ItemSpacing.x * 3;

        const bool show_bar = g::pOptsManager->stat_as_progress_load();

        ImGui::BeginGroup();

        ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::StatSepCapt), 0.0f, server.data(), server.size(), server_wid);
        if (!g::pFlow->get_online())
        {
            ImGui::TextUnformatted("Status");
            ImGui::SameLine(val_text_offset);
            ImGui::TextUnformatted("offline");
        }
        else [[likely]]
        {
            ImGui::TextUnformatted("Status");
            ImGui::SameLine(val_text_offset);
            if (m_queue_remaining.load(std::memory_order_relaxed) > 0)
            {
                m_buffer.clear();
                fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("Executing: {}"), m_queue_remaining.load(std::memory_order_relaxed));
                ImGui::TextUnformatted(m_buffer.data(), m_buffer.data() + m_buffer.size());
            }
            else
                ImGui::TextUnformatted("Idle");

            auto stats = m_server_stats.load(std::memory_order_acquire);

            ImGui::TextUnformatted("Version");
            ImGui::SameLine(val_text_offset);
            if (stats)
                ImGui::TextUnformatted(stats->comfy_ver.c_str());

            print_ping(val_text_offset);

            if (stats)
            {
                if (show_bar)
                    show_server_as_bar(val_text_offset, stats.get());
                else
                    show_server_as_text(val_text_offset, stats.get());
            }
        }

        ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::StatSepCapt), 0.0f, this_pc.data(), this_pc.size(), this_pc_wid);

        if (show_bar)
            show_this_pc_as_bar(val_text_offset);
        else
            show_this_pc_as_text(val_text_offset);        

        if (g::pOptsManager->app_show_fps_load())
        {
            ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::StatSepCapt), 0.0f, performance.data(), performance.size(), performance_wid);
            //show_app_performance(val_text_offset);
            print_fps(val_text_offset);
        }

        ImGui::EndGroup();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            g::pOptsManager->stat_as_progress_save(!show_bar);
    }
    else
        m_pause.store(true, std::memory_order_relaxed);

    return end_panel(opened);
}

void PanelStats::print_text_stat(const char* stat_name, float val_offset, uint64_t value, int64_t lim_warn, int64_t lim_crit, bool is_lower_better, bool do_format_bytes)
{
    bool def_clr = true;
    const auto color = get_color(value, def_clr, lim_warn, lim_crit, is_lower_better);
    if (do_format_bytes)
        format_bytes(value);

    if (!def_clr)
        ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(stat_name);
    ImGui::SameLine(val_offset);
    if (do_format_bytes)
        ImGui::TextUnformatted(m_buffer.data(), m_buffer.data() + m_buffer.size());
    else
        ImGui::Text("%llu", value);
    if (!def_clr)
        ImGui::PopStyleColor();
};

void PanelStats::show_this_pc_as_text(float val_offset)
{
    print_text_stat("RAM", val_offset, m_lram_free.load(std::memory_order_relaxed),
        g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalRAM),
        g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalRAM),
        false, true);

    for (auto& disk : m_disks)
    {
        if (disk.disk_opt->second)
        {
            const uint64_t disk_free = disk.free.load(std::memory_order_relaxed);
            print_text_stat(disk.format_path.c_str(), val_offset, disk_free,
                g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalDisk),
                g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalDisk),
                false, true);
        }
    }
}

void PanelStats::show_server_as_text(float val_offset, const ServerStats* stats)
{
    if (stats->sram_free > 0)
    {
        print_text_stat("RAM", val_offset,
            stats->sram_free,
            g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerRAM),
            g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerRAM),
            false, true);
    }

    for (auto& vd : stats->video_devices)
    {
        if (vd.pytorch_total > 0 || vd.pytorch_free > 0 || vd.vram_free > 0)
        {
            ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::StatSepSubtitle), 0.0f, vd.name.c_str(), vd.name.size(), 0.0f);

            if (vd.pytorch_total > 0)
            {
                ImGui::TextUnformatted("PyTorch total");
                ImGui::SameLine(val_offset);
                format_bytes(vd.pytorch_total);
                ImGui::TextUnformatted(m_buffer.data(), m_buffer.data() + m_buffer.size());
            }

            if (vd.pytorch_free > 0)
            {
                print_text_stat("PyTorch free", val_offset, vd.pytorch_free,
                    g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerPyTorch),
                    g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerPyTorch),
                    false, true);
            }
            if (vd.vram_free > 0)
            {
                print_text_stat("VRAM", val_offset, vd.vram_free,
                    g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerVRAM),
                    g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerVRAM),
                    false, true);
            }
        }
    }
}

void PanelStats::print_fps(float val_offset)
{
    //const float instant_fps = ImGui::GetIO().Framerate;
    static float accumulated_time = 0.0f;
    static int32_t frame_counter = 0;
    static FrameSmoother smoother;

    accumulated_time += ImGui::GetIO().DeltaTime;
    frame_counter++;

    if (accumulated_time >= 0.5f)
    {
        m_fps = cast::to_i64(cast::to_float(frame_counter) / accumulated_time);
        accumulated_time = 0.0f;
        frame_counter = 0;
    }

    const auto latency = smoother.update(g::pMain->get_frame_render_time());

    ImU32 color = 0;
    if (latency >= g::pMain->get_frame_budget_crit())
        color = g::pOptsManager->color_load(AppColor::StatCrit);
    else if (latency >= g::pMain->get_frame_budget_warn())
        color = g::pOptsManager->color_load(AppColor::StatWarn);

    //const auto color = get_color(cast::to_i64(latency * 100), def_clr,
    //    g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalFPS),
    //    g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalFPS),
    //    true);

    if (color != 0)
        ImGui::PushStyleColor(ImGuiCol_Text, color);

    ImGui::TextUnformatted("Latency");
    ImGui::SameLine(val_offset);
    ImGui::Text("%.1f ms", latency);

    ImGui::TextUnformatted("FPS");
    ImGui::SameLine(val_offset);
    ImGui::Text("%llu", m_fps);

    if (color != 0)
        ImGui::PopStyleColor();
}

void PanelStats::print_ping(float val_offset) noexcept
{
    bool def_clr = true;
    const auto color = get_color(m_ping_mk / 1000, def_clr,
        g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerPing),
        g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerPing),
        true);

    if (!def_clr)
        ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted("Ping");
    ImGui::SameLine(val_offset);
    if (m_ping_mk > 0) [[likely]]
    {
        if (m_ping_mk < 1000)
            ImGui::Text("%.3f ms", cast::to_float(m_ping_mk) / 1000.0f);
        else if (m_ping_mk < 1000000)
            ImGui::Text("%.2f ms", cast::to_float(m_ping_mk) / 1000.0f);
        else
            ImGui::Text("%.2f s", cast::to_float(m_ping_mk) / 1000000.0f);

        if (cast::to_float(m_ping_mk) / 1000.0f > g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerPing))
        {
            ImGui::SameLine();
            ImGui::TextUnformatted("[?]");
            ImGuiEx::TooltipOnLastControl(
                "Critical latency detected.\n"
                "The backend appears unresponsive. It might be frozen, crashed,\n"
                "or blocked by a heavy non-interruptible task.", nullptr);
        }
    }
    else
        ImGui::TextUnformatted("---");
    if (!def_clr)
        ImGui::PopStyleColor();
}

void PanelStats::show_app_performance(float val_offset)
{
    //ImGui::TextUnformatted("V-Sync");
    //ImGui::SameLine(val_offset);
    //ImGui::TextUnformatted(g::pOptsManager->app_vsync_load() ? "enabled" : "disabled");

    print_fps(val_offset);
}

void PanelStats::print_bar_stat(const char* stat_name, float val_offset, uint64_t free, uint64_t total, int64_t lim_warn, int64_t lim_crit, const char* tt)
{
    if (total > 0)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(stat_name);
        ImGui::SameLine(val_offset);

        m_buffer.clear();
        fmt::format_to(std::back_inserter(m_buffer),
            "{}\n"
            "Capacity:  {:.2f} GB\n"
            "Available: {:.2f} GB",
            tt,
            cast::to_double(total) / 1024 / 1024 / 1024,
            cast::to_double(free) / 1024 / 1024 / 1024
        );

        bool def;
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, get_color(free, def, lim_warn, lim_crit, false));
        const float fraction = cast::to_float(cast::to_double(total - free) / cast::to_double(total));
        ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0.0f), "");
        ImGui::PopStyleColor();
        ImGuiEx::TooltipOnLastControl(m_buffer.data(), m_buffer.data() + m_buffer.size(), ImGuiHoveredFlags_DelayNormal);
    }
}

void PanelStats::show_this_pc_as_bar(float val_offset)
{
    print_bar_stat("RAM", val_offset,
        m_lram_free.load(std::memory_order_relaxed),
        m_lram_total.load(std::memory_order_relaxed),
        g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalRAM),
        g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalRAM),
        "Local RAM");

    for (auto& disk : m_disks)
    {
        if (disk.disk_opt->second)
        {
            const uint64_t disk_free = disk.free.load(std::memory_order_relaxed);
            const uint64_t disk_total = disk.total.load(std::memory_order_relaxed);
            print_bar_stat(disk.format_path.c_str(), val_offset, disk_free, disk_total,
                g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalDisk),
                g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalDisk),
                disk.format_path.c_str());
        }
    }
}

void PanelStats::show_server_as_bar(float val_offset, const ServerStats* stats)
{
    print_bar_stat("RAM", val_offset,
        stats->sram_free, stats->sram_total,
        g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerRAM),
        g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerRAM),
        "Server RAM");

    for (auto& vd : stats->video_devices)
    {
        if (vd.pytorch_total > 0 || vd.pytorch_free > 0 || vd.vram_free > 0)
        {
            ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::StatSepSubtitle), 0.0f, vd.name.c_str(), vd.name.size(), 0.0f);

            print_bar_stat("VRAM", val_offset, vd.vram_free, vd.vram_total,
                g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerVRAM),
                g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerVRAM),
                "Video RAM");

            print_bar_stat("PyTorch VRAM", val_offset, vd.pytorch_free, vd.pytorch_total,
                g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerPyTorch),
                g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerPyTorch),
                "PyTorch Video RAM");
        }
    }
}

} // namespace ui
