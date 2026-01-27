
#include "ui/panels/panel_stats.h"
#include "ui/graph-n_flow.h"
#include "platform/main.h"

namespace ui
{

PanelStats::PanelStats()
    : PanelBase(DlgObj::PanelStats, ICON_LC_CHART_BAR " Stats", ICON_LC_CHART_BAR, "29C805C1-AA12-4FC2-9954-F7849E5DBFDA")
{}

PanelStats::~PanelStats()
{
    LogF;
}

void PanelStats::init()
{
    LogF;
    PanelBase::init();

    init_disks();

    m_stop = false;
    m_pause = false;
    m_th_api = std::thread(&PanelStats::th_api, this);
    m_th_comfyui = std::thread(&PanelStats::th_comfyui, this);
    m_th_ws = std::thread(&PanelStats::th_ws, this);
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
        d.format_path = fmt::format("Disk {}", d.disk_opt->first);
#else
        d.format_path = d.path;
#endif
        m_disks.push_back(std::move(d));
    }
}

void PanelStats::stop_all_threads()
{
    LogF_In;
    m_pause = false;
    m_stop = true;
    if (m_th_api.joinable())  m_th_api.join();
    if (m_th_comfyui.joinable())  m_th_comfyui.join();
    if (m_th_ws.joinable())  m_th_ws.join();
    LogF_Out;
}

void PanelStats::th_api()
{
    using namespace std::chrono_literals;

    const auto check_ram_interval = 1000ms;
    const auto check_disk_interval = 5000ms;
    const auto sleep_step = 150ms;
    auto accumulated_time_disk = 0ms;
    auto accumulated_time_ram = 0ms;

    while (!m_stop)
    {
        std::this_thread::sleep_for(sleep_step);
        if (m_stop)
            break;
        if (m_pause)
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
            if (GlobalMemoryStatusEx(&info))
            {
                m_lram_free = info.ullAvailPhys;
                m_lram_total = info.ullTotalPhys;
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
                if (GetDiskFreeSpaceExA(disk.disk_opt->first.c_str(), &free_bytes, &total_bytes, &total_free))
                {
                    std::lock_guard<std::mutex> lock(m_lock);
                    disk.free = free_bytes.QuadPart;
                    disk.total = total_bytes.QuadPart;
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
    using namespace std::chrono_literals;

    const auto check_interval = 2500ms;
    const auto sleep_step = 150ms;
    auto accumulated_time = 0ms;

    bool system_stats_err = false;
    bool prompt_err = false;

    while (!m_stop)
    {
        std::this_thread::sleep_for(sleep_step);
        if (m_stop)
            break;
        if (m_pause || !g::pFlow->get_online())
        {
            accumulated_time = check_interval;
            continue;
        }

        accumulated_time += sleep_step;
        if (accumulated_time >= check_interval)
        {
            httplib::Client cli(g::pOptsManager->comfyui_host_load(), cast::to_i32(g::pOptsManager->comfyui_port_load()));
            auto res = cli.Get(comfyui_api::system_stats);
            if (res && 200 == res->status)
            {
                try
                {
                    const auto data = nlohmann::json::parse(res->body);
                    if (data.contains("system") && data["system"].contains("ram_free") && data["system"]["ram_free"].is_number_integer())
                    {
                        auto& system = data["system"];
                        if (system.contains("ram_free") && system["ram_free"].is_number_integer())
                            m_sram_free = system["ram_free"];
                        if (system.contains("ram_total") && system["ram_total"].is_number_integer())
                            m_sram_total = system["ram_total"];
                    }
                    if (data.contains("devices") && data["devices"].is_array())
                    {
                        std::list<VideoDevice> list;

                        auto& devices = data["devices"];
                        for (auto& device : devices)
                        {
                            VideoDevice vd;
                            if (device.contains("name") && device["name"].is_string())
                            {
                                vd.name = device["name"];

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
                            if (device.contains("vram_free") && device["vram_free"].is_number_integer())
                            {
                                vd.vram_free = device["vram_free"];
                            }
                            if (device.contains("vram_total") && device["vram_total"].is_number_integer())
                            {
                                vd.vram_total = device["vram_total"];
                            }
                            if (device.contains("torch_vram_total") && device["torch_vram_total"].is_number_integer())
                            {
                                vd.pytorch_total = device["torch_vram_total"];
                            }
                            if (device.contains("torch_vram_free") && device["torch_vram_free"].is_number_integer())
                            {
                                vd.pytorch_free = device["torch_vram_free"];
                            }
                            list.push_back(std::move(vd));
                        }

                        std::lock_guard<std::mutex> lock(m_lock);
                        m_video_devices = std::move(list);
                    }
                }
                catch (std::exception& e)
                {
                    if (!system_stats_err)
                        LogExcep(ExcepCategory::ComfyUI, "/system_stats parse error: " << e.what());
                    system_stats_err = true;
                }
            }

            res = cli.Get(comfyui_api::prompt);
            if (res && 200 == res->status)
            {
                try
                {
                    const auto data = nlohmann::json::parse(res->body);
                    if (data.contains("exec_info") && data["exec_info"].contains("queue_remaining") && data["exec_info"]["queue_remaining"].is_number_integer())
                    {
                        m_queue_remaining = data["exec_info"]["queue_remaining"];
                    }
                }
                catch (std::exception& e)
                {
                    if (!prompt_err)
                        LogExcep(ExcepCategory::ComfyUI, "/prompt parse error: " << e.what());
                    prompt_err = true;
                }
            }

            accumulated_time = 0ms;
        }
    }
}

void PanelStats::th_ws()
{
    //using namespace std::chrono_literals;

    //const auto check_interval_idle = 2000ms;
    //const auto check_interval_executing = 1000ms;
    //const auto sleep_step = 150ms;
    //auto accumulated_time_idle = 0ms;
    //auto accumulated_time_exec = 0ms;
    //auto accumulated_time_ping = 0ms;

    //while (!m_stop)
    //{
    //    std::this_thread::sleep_for(sleep_step);
    //    if (m_stop) break;

    //    accumulated_time += sleep_step;
    //    if (accumulated_time >= check_interval)
    //    {
    //        // Do calculations
    //        // ...

    //        accumulated_time = 0ms;
    //    }
    //}
}

struct FrameSmoother final
{
    double values[60] = {};
    int32_t offset = 0;

    float update(double new_val)
    {
        values[offset] = new_val;
        offset = (offset + 1) % 60;

        double sum = 0;
        for(double v : values)
            sum += v;
        return cast::to_float(sum / 60);
    }
};

ImU32 PanelStats::get_color(int64_t value, bool& stay_default, int64_t lim_warn, int64_t lim_crit, bool is_lower_better)
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
    fmt::format_to(std::back_inserter(m_buffer), "{:.2f} {}", count, suffixes[suffix_index]);
    *(m_buffer.data() + m_buffer.size()) = 0;
};

DlgCode PanelStats::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        m_pause = false;

        static constexpr const std::string_view this_pc = "This PC";
        static const float this_pc_wid = ImGui::CalcTextSize(this_pc.data()).x;

        static constexpr const std::string_view server = "Server";
        static const float server_wid = ImGui::CalcTextSize(server.data()).x;

        static constexpr const std::string_view queue = "Queue";
        static const float queue_wid = ImGui::CalcTextSize(queue.data()).x;

        static constexpr const std::string_view performance = "Performance";
        static const float performance_wid = ImGui::CalcTextSize(performance.data()).x;

        static const float val_text_offset = ImGui::CalcTextSize("PyTorch total").x + ImGui::GetStyle().ItemSpacing.x * 3;

        const bool show_bar = g::pOptsManager->stat_as_progress_load();

        ImGui::BeginGroup();

        if (m_queue_remaining > 0)
        {
            ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::StatSepSubtitle), 0.0f, queue.data(), queue.size(), queue_wid);
            show_queue(val_text_offset);            
        }

        ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::StatSepCapt), 0.0f, server.data(), server.size(), server_wid);
        if (!g::pFlow->get_online())
        {
            ImGui::TextUnformatted("Status");
            ImGui::SameLine(val_text_offset);
            ImGui::TextUnformatted("offline");
        }
        else
        {
            ImGui::TextUnformatted("Status");
            ImGui::SameLine(val_text_offset);
            ImGui::TextUnformatted(m_queue_remaining > 0 ? "Executing" : "Idle");

            if (show_bar)
                show_server_as_bar(val_text_offset);
            else
                show_server_as_text(val_text_offset);            
        }

        ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::StatSepCapt), 0.0f, this_pc.data(), this_pc.size(), this_pc_wid);

        if (show_bar)
            show_this_pc_as_bar(val_text_offset);
        else
            show_this_pc_as_text(val_text_offset);        

        ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::StatSepCapt), 0.0f, performance.data(), performance.size(), performance_wid);
        show_performance(val_text_offset);

        ImGui::EndGroup();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            g::pOptsManager->stat_as_progress_save(!show_bar);
    }
    else
        m_pause = true;

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
        ImGui::Text("%u", value);
    if (!def_clr)
        ImGui::PopStyleColor();
};

void PanelStats::show_this_pc_as_text(float val_offset)
{
    print_text_stat("RAM", val_offset, m_lram_free,
        g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalRAM),
        g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalRAM),
        false, true);

    for (auto& disk : m_disks)
    {
        if (disk.disk_opt->second)
        {
            print_text_stat(disk.format_path.c_str(), val_offset, disk.free,
                g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalDisk),
                g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalDisk),
                false, true);
        }
    }
}

void PanelStats::show_server_as_text(float val_offset)
{
    if (m_ping > 0)
    {
        print_text_stat("Ping", val_offset, m_ping,
            g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerPing),
            g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerPing),
            true, false);
    }

    if (m_sram_free > 0)
    {
        print_text_stat("RAM", val_offset, m_sram_free,
            g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerRAM),
            g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerRAM),
            false, true);
    }

    std::list<VideoDevice> devices;
    {
        std::lock_guard<std::mutex> lock(m_lock);
        devices = m_video_devices;
    }
    for (auto& vd : devices)
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
    const float instant_fps = ImGui::GetIO().Framerate;
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

    bool def_clr = true;
    const auto color = get_color(cast::to_i64(latency * 100), def_clr,
        g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalFPS),
        g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalFPS),
        true);

    if (!def_clr)
        ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted("Latency");
    ImGui::SameLine(val_offset);
    ImGui::Text("%.1f ms, FPS: %u", latency, m_fps);
    if (!def_clr)
        ImGui::PopStyleColor();
}

void PanelStats::show_performance(float val_offset)
{
    ImGui::TextUnformatted("V-Sync");
    ImGui::SameLine(val_offset);
    ImGui::TextUnformatted(g::pOptsManager->app_vsync_load() ? "enabled" : "disabled");

    print_fps(val_offset);

    if (m_ping > 0)
    {
        print_text_stat("Ping", val_offset, m_ping,
            g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerPing),
            g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerPing),
            true, false);
    }

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
    print_bar_stat("RAM", val_offset, m_lram_free, m_lram_total,
        g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalRAM),
        g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalRAM),
        "Local RAM");

    for (auto& disk : m_disks)
    {
        if (disk.disk_opt->second)
        {
            print_bar_stat(disk.format_path.c_str(), val_offset, disk.free, disk.total,
                g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalDisk),
                g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalDisk),
                disk.format_path.c_str());
        }
    }
}

void PanelStats::show_server_as_bar(float val_offset)
{
    print_bar_stat("RAM", val_offset, m_sram_free, m_sram_total,
        g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerRAM),
        g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerRAM),
        "Server RAM");

    std::list<VideoDevice> devices;
    {
        std::lock_guard<std::mutex> lock(m_lock);
        devices = m_video_devices;
    }
    for (auto& vd : devices)
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

void PanelStats::show_queue(float /*val_offset*/)
{
}

} // namespace ui
