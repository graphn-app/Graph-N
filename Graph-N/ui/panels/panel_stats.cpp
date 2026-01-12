
#include "ui/panels/panel_stats.h"
#include "ui/graph-n_flow.h"
#include "core/managers/settings_manager.h"
#include "platform/main.h"

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

    g::pOptsManager->stat_get_lram_threshold(m_lim_lram.warning, m_lim_lram.critical);
    g::pOptsManager->stat_get_sram_threshold(m_lim_sram.warning, m_lim_sram.critical);
    g::pOptsManager->stat_get_vram_threshold(m_lim_vram.warning, m_lim_vram.critical);
    g::pOptsManager->stat_get_pytorch_threshold(m_lim_pytorch.warning, m_lim_pytorch.critical);
    g::pOptsManager->stat_get_disk_threshold(m_lim_disk.warning, m_lim_disk.critical);
    g::pOptsManager->stat_get_fps_threshold(m_lim_fps.warning, m_lim_fps.critical);
    g::pOptsManager->stat_get_ping_threshold(m_lim_ping.warning, m_lim_ping.critical);
    m_lim_ping.is_lower_better = true;
    init_disks();

    m_stop = false;
    m_pause = false;
    m_th_api = std::thread(&PanelStats::th_api, this);
    m_th_comfyui = std::thread(&PanelStats::th_comfyui, this);
    m_th_ws = std::thread(&PanelStats::th_ws, this);
}

void PanelStats::init_disks()
{
    const auto enabled_disks = g::pOptsManager->stat_get_disks_enabled();

    // get disks list
#ifdef _WIN32
    char buffer[512];
    const DWORD len = GetLogicalDriveStringsA(sizeof(buffer), buffer);
    if (0 == len)
    {
        LogError(ExcepCategory::api, "GetLogicalDriveStringsA failed, err " << LogERR);
        return;
    }

    const char* drive = buffer;
    while (*drive)
    {
        // Filter: Only fixed drives (HDD/SSD). Ignore USB/Network to avoid lag.
        if (DRIVE_FIXED == GetDriveTypeA(drive))
        {
            Disks info;
            info.path = drive;
            info.format_path = fmt::format("Disk {}", drive);
            const auto it = std::find_if(enabled_disks.begin(), enabled_disks.end(), [&info](const std::string& path) { return path == info.path; });
            info.enabled = enabled_disks.empty() || it != enabled_disks.end();
            m_disks.push_back(info);
        }
        drive += strlen(drive) + 1;
    }
#elif defined __linux__
#error Not implemented
#elif defined __APPLE__
#error Not implemented
#endif

    if (!enabled_disks.empty())
    {
        for (auto& disk : m_disks)
        {
            const auto it = std::find_if(enabled_disks.begin(), enabled_disks.end(), [&disk](const std::string& path) { return path == disk.path; });
            disk.enabled = it != enabled_disks.end();
        }
    }
}

void PanelStats::stop_all_threads()
{
    m_pause = false;
    m_stop = true;
    if (m_th_api.joinable())  m_th_api.join();
    if (m_th_comfyui.joinable())  m_th_comfyui.join();
    if (m_th_ws.joinable())  m_th_ws.join();
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
                m_lram = info.ullAvailPhys;
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
                if (!disk.enabled)
                    continue;

                ULARGE_INTEGER free_bytes = {}, total_bytes = {}, total_free = {};
                if (GetDiskFreeSpaceExA(disk.path.c_str(), &free_bytes, &total_bytes, &total_free))
                {
                    std::lock_guard<std::mutex> lock(m_lock);
                    disk.free = free_bytes.QuadPart;
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
            const auto host = g::pOptsManager->comfyui_get_host();
            const auto port = g::pOptsManager->comfyui_get_port();

            httplib::Client cli(host, port);
            auto res = cli.Get(comfyui_api::system_stats);
            if (res && 200 == res->status)
            {
                try
                {
                    const auto data = nlohmann::json::parse(res->body);
                    if (data.contains("system") && data["system"].contains("ram_free") && data["system"]["ram_free"].is_number_integer())
                    {
                        m_sram = data["system"]["ram_free"];
                    }
                    if (data.contains("devices") && data["devices"].is_array())
                    {
                        std::list<VideoDevice> list;

                        auto& devices = data["devices"];
                        for (auto& device : devices)
                        {
                            list.push_back({});
                            VideoDevice& vd = list.back();
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
                                vd.vram = device["vram_free"];
                            }
                            if (device.contains("torch_vram_total") && device["torch_vram_total"].is_number_integer())
                            {
                                vd.pytorch_total = device["torch_vram_total"];
                            }
                            if (device.contains("torch_vram_free") && device["torch_vram_free"].is_number_integer())
                            {
                                vd.pytorch_free = device["torch_vram_free"];
                            }
                        }

                        std::lock_guard<std::mutex> lock(m_lock);
                        m_video_devices = std::move(list);
                    }
                }
                catch (std::exception& e)
                {
                    if (!system_stats_err)
                        LogExcep(ExcepCategory::comfyui, "/system_stats parse error: " << e.what());
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
                        LogExcep(ExcepCategory::comfyui, "/prompt parse error: " << e.what());
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

void PanelStats::calc_fps()
{
    const float instant_fps = ImGui::GetIO().Framerate;
    static float accumulated_time = 0.0f;
    static int32_t frame_counter = 0;

    accumulated_time += ImGui::GetIO().DeltaTime;
    frame_counter++;

    if (accumulated_time >= 0.5f)
    {
        m_fps = cast::to_i64(cast::to_float(frame_counter) / accumulated_time);
        accumulated_time = 0.0f;
        frame_counter = 0;
    }
}

ImU32 PanelStats::get_color(int64_t value, bool& stay_default, const MetricLimits& lim)
{
    stay_default = false;

    if (value > 0)
    {
        constexpr ImU32 clr_warn = IM_COL32(255, 200, 50, 255); // Yellow
        constexpr ImU32 clr_crit = IM_COL32(255, 50, 50, 255);  // Red

        if (lim.is_lower_better)
        {
            if (value >= lim.critical) return clr_crit;
            if (value >= lim.warning) return clr_warn;
        }
        else
        {
            if (value <= lim.critical) return clr_crit;
            if (value <= lim.warning) return clr_warn;
        }
    }

    stay_default = true;
    return 0;
}

DlgCode PanelStats::show(bool& focused)
{
    const auto format_bytes = [this](uint64_t bytes)
    {        
        static constexpr std::string_view suffixes[] = { "B", "KB", "MB", "GB", "TB", "PB" };
        size_t suffix_index = 0;
        double count = cast::to_double(cast::to_i64(bytes));

        while (count >= 1024.0 && suffix_index < 5)
        {
            count /= 1024.0;
            suffix_index++;
        }

        m_buffer.clear();
        fmt::format_to(std::back_inserter(m_buffer), "{:.2f} {}", count, suffixes[suffix_index]);
    };

    const auto print_stat = [this, &format_bytes](const char* stat_name, float val_offset, uint64_t value, const MetricLimits& limits, bool do_format_bytes = true)
    {
        bool def_clr = true;
        const auto color = get_color(value, def_clr, limits);
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

    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
    {
        m_pause = false;
        calc_fps();

        static constexpr const std::string_view this_pc = "This PC";
        static const float this_pc_wid = ImGui::CalcTextSize(this_pc.data()).x;

        static constexpr const std::string_view server = "Server";
        static const float server_wid = ImGui::CalcTextSize(server.data()).x;

        static constexpr const std::string_view queue = "Queue";
        static const float queue_wid = ImGui::CalcTextSize(queue.data()).x;

        static constexpr const std::string_view ram_local = "RAM";
        static constexpr const std::string_view ram_server = "RAM";
        static constexpr const std::string_view vsync = "V-Sync";
        static constexpr const std::string_view fps = "FPS";
        static constexpr const std::string_view status = "Status";
        static constexpr const std::string_view vram_server = "VRAM";
        static constexpr const std::string_view vram_pytorch_total = "PyTorch total";
        static constexpr const std::string_view vram_pytorch_free = "PyTorch free";
        static constexpr const std::string_view ping = "Ping";

        static float offset = 0.0f;
        if (0.0f == offset)
        {
            offset = std::max(offset, ImGui::CalcTextSize(ram_local.data()).x);
            offset = std::max(offset, ImGui::CalcTextSize(ram_server.data()).x);
            offset = std::max(offset, ImGui::CalcTextSize(fps.data()).x);
            offset = std::max(offset, ImGui::CalcTextSize(status.data()).x);
            offset = std::max(offset, ImGui::CalcTextSize(vsync.data()).x);
            offset = std::max(offset, ImGui::CalcTextSize(vram_pytorch_total.data()).x);
            offset = std::max(offset, ImGui::CalcTextSize(vram_pytorch_free.data()).x);
            offset = std::max(offset, ImGui::CalcTextSize(ping.data()).x);

            for (auto& disk : m_disks)
            {
                offset = std::max(offset, cast::to_float(cast::to_i64(disk.format_path.size())));
            }

            offset += ImGui::GetStyle().ItemSpacing.x * 4;
        }

        ImGuiEx::Separator(IM_COL32(0, 180, 180, 200), 0.0f, this_pc.data(), this_pc.size(), this_pc_wid);

        ImGui::TextUnformatted(vsync.data(), vsync.data() + vsync.size());
        ImGui::SameLine(offset);
        bool vsync_enabled = g::pMain->get_vsync_enabled();
        if (ImGui::Checkbox(vsync_enabled ? "Enabled##VSync" : "Disabled##VSync", &vsync_enabled))
        {
            g::pOptsManager->app_set_vsync_enabled(vsync_enabled);
            g::pMain->set_vsync_enabled(vsync_enabled);
        }

        print_stat(fps.data(), offset, m_fps, m_lim_fps, false);
        print_stat(ram_local.data(), offset, m_lram, m_lim_lram);

        for (auto& disk : m_disks)
        {
            if (disk.enabled)
            {
                print_stat(disk.format_path.c_str(), offset, disk.free, m_lim_disk);
            }
        }

        ImGuiEx::Separator(IM_COL32(0, 180, 180, 200), 0.0f, server.data(), server.size(), server_wid);
        if (!g::pFlow->get_online())
        {
            ImGui::TextUnformatted(status.data(), status.data() + status.size());
            ImGui::SameLine(offset);
            ImGui::TextUnformatted("Offline");
        }
        else
        {
            ImGui::TextUnformatted(status.data(), status.data() + status.size());
            ImGui::SameLine(offset);
            ImGui::TextUnformatted(m_queue_remaining > 0 ? "Executing" : "Idle");                

            if (m_ping > 0)
            {
                print_stat(ping.data(), offset, m_ping, m_lim_ping);
            }

            if (m_sram > 0)
            {
                print_stat(ram_server.data(), offset, m_sram, m_lim_sram);
            }

            std::list<VideoDevice> devices;
            {
                std::lock_guard<std::mutex> lock(m_lock);
                devices = m_video_devices;
            }
            for (auto& vd : devices)
            {
                if (vd.pytorch_total > 0 || vd.pytorch_free > 0 || vd.vram > 0)
                {
                    ImGuiEx::Separator(IM_COL32(255, 255, 255, 200), 0.0f, vd.name.c_str(), vd.name.size(), 0.0f);

                    if (vd.pytorch_total > 0)
                    {
                        ImGui::TextUnformatted(vram_pytorch_total.data(), vram_pytorch_total.data() + vram_pytorch_total.size());
                        ImGui::SameLine(offset);
                        format_bytes(vd.pytorch_total);
                        ImGui::TextUnformatted(m_buffer.data(), m_buffer.data() + m_buffer.size());
                    }

                    if (vd.pytorch_free > 0)
                    {
                        print_stat(vram_pytorch_free.data(), offset, vd.pytorch_free, m_lim_pytorch);
                    }
                    if (vd.vram > 0)
                    {
                        print_stat(vram_server.data(), offset, vd.vram, m_lim_vram);
                    }
                }
            }

            if (m_queue_remaining > 0)
            {
                ImGuiEx::Separator(IM_COL32(255, 255, 255, 200), 0.0f, queue.data(), queue.size(), queue_wid);
            }
        }
    }
    else
        m_pause = true;

    return end_panel(opened);
}

void PanelStats::show_config_menu()
{
    constexpr const float sep_w = 200.0f;
    constexpr const auto sep_clr = IM_COL32(0, 180, 180, 200);
    constexpr const float slider_w = 120.0f;

    const auto show_thresholds = [sep_w, sep_clr, slider_w](
        const char* sep_name, const char* table_name, const char* warn_name, const char* crit_name,
        MetricLimits& lim,
        float min, float max, const char* format)
    {
        bool chnaged = false;
        ImGuiEx::Separator(sep_clr, sep_w, sep_name);
        if (ImGui::BeginTable(table_name, 2, ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Warn:");

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(slider_w);
            float val = cast::to_float(lim.warning);
            if (ImGui::SliderFloat(warn_name, &val, min, max, format))
            {
                lim.warning = cast::to_i64(val);
                chnaged = true;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Crit:");

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(slider_w);
            val = cast::to_float(lim.critical);
            if (ImGui::SliderFloat(crit_name, &val, min, max, format))
            {
                lim.critical = cast::to_i64(val);
                chnaged = true;
            }

            ImGui::EndTable();
        }

        return chnaged;
    };

    if (ImGui::BeginPopupContextItem("##fpsmenu"))
    {
        show_thresholds("This PC: FPS thresholds", "##fpstable", "FPS##Warning", "FPS##Critical", m_lim_fps, 5.0f, 60.0f, "%.0f");
        show_thresholds("This PC: RAM thresholds", "##lramtable", "GB##LRAMWarning", "GB##LRAMCritical", m_lim_lram, 0.5f, 10.0f, "%.1f");
        show_thresholds("This PC: Disk thresholds", "##diskstable", "GB##DiskWarning", "GB##DiskCritical", m_lim_disk, 1.0f, 40.0f, "%.0f");

        for (auto& disk : m_disks)
        {
            if (ImGui::Checkbox(disk.format_path.c_str(), &disk.enabled))
            {
                std::vector<std::string> enabled_disks;
                for (auto& d : m_disks)
                {
                    if (d.enabled)
                        enabled_disks.push_back(d.path);
                }
                if (enabled_disks.empty())
                    enabled_disks.push_back("(.)(.)");
                g::pOptsManager->stat_set_disks_enabled(enabled_disks);
            }
        }

        show_thresholds("Server: RAM thresholds", "##sramtable", "GB##SRAMWarning", "GB##SRAMCritical", m_lim_sram, 1.0f, 10.0f, "%.1f");
        show_thresholds("Server: VRAM thresholds", "##vramtable", "GB##VRAMWarning", "GB##VRAMCritical", m_lim_vram, 0.1f, 5.0f, "%.1f");
        show_thresholds("Server: PyTorch VRAM thresholds", "##PyTorchtable", "GB##PyTorchWarning", "GB##PyTorchCritical", m_lim_pytorch, 0.1f, 5.0f, "%.1f");
        show_thresholds("Server: Ping thresholds", "##Pingtable", "ms##pingWarning", "GB##PingCritical", m_lim_ping, 50.0f, 500.0f, "%.0f");

        ImGui::EndPopup();
    }
}

} // namespace ui
