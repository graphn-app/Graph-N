
#include "ui/options/opt_stats_dlg.h"
#include "core/managers/settings_manager.h"

namespace ui
{

enum
{
    buff_fps_w,
    buff_fps_c,
    buff_lram_w,
    buff_lram_c,
    buff_sram_w,
    buff_sram_c,
    buff_vram_w,
    buff_vram_c,
    buff_disk_w,
    buff_disk_c,
    buff_pytorch_w,
    buff_pytorch_c,
    buff_ping_w,
    buff_ping_c,

    _buff_i_count,
};

OptStatsDlg::~OptStatsDlg()
{
    LogF;
}

void OptStatsDlg::init(OptsData& data)
{
    LogF;

    data.stats.buffer_i->resize(_buff_i_count);
    (*data.stats.buffer_i)[buff_fps_w] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::local_fps);
    (*data.stats.buffer_i)[buff_fps_c] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::local_fps);
    (*data.stats.buffer_i)[buff_lram_w] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::local_ram);
    (*data.stats.buffer_i)[buff_lram_c] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::local_ram);
    (*data.stats.buffer_i)[buff_sram_w] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::server_ram);
    (*data.stats.buffer_i)[buff_sram_c] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::server_ram);
    (*data.stats.buffer_i)[buff_vram_w] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::server_vram);
    (*data.stats.buffer_i)[buff_vram_c] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::server_vram);
    (*data.stats.buffer_i)[buff_disk_w] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::local_disk);
    (*data.stats.buffer_i)[buff_disk_c] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::local_disk);
    (*data.stats.buffer_i)[buff_pytorch_w] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::server_pytorch);
    (*data.stats.buffer_i)[buff_pytorch_c] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::server_pytorch);
    (*data.stats.buffer_i)[buff_ping_w] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::server_ping);
    (*data.stats.buffer_i)[buff_ping_c] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::server_ping);
}

void OptStatsDlg::show(OptsData& data)
{
    auto& buffer_i = *data.stats.buffer_i;

    const auto bytes_to_gb = [](int64_t bytes) -> double
    {
        return cast::to_double(bytes) / (1024.0 * 1024.0 * 1024.0);
    };

    const auto gb_to_bytes = [](double gb) -> int64_t
    {
        return cast::to_i64(gb * 1024.0 * 1024.0 * 1024.0);
    };

    const auto show_thresholds = [&bytes_to_gb, &gb_to_bytes](
        const char* table_name, const char* warn_name, const char* crit_name,
        int64_t& warn, int64_t& crit, bool convert_bytes, bool div_by_100,
        float min, float max, const char* format)
    {
        std::pair<bool, bool> changed = std::make_pair(false, false);
        if (ImGui::BeginTable(table_name, 2, ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Warning:");

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            double val = convert_bytes ? bytes_to_gb(warn) : div_by_100 ? cast::to_double(warn) / 100 : cast::to_double(warn);
            if (ImGuiEx::SliderDoubleSmart(warn_name, &val, min, max, format))
                warn = convert_bytes ? gb_to_bytes(val) : div_by_100 ? cast::to_i64(val * 100) : cast::to_i64(val);
            changed.first = ImGui::IsItemDeactivatedAfterEdit();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Critical:");

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            val = convert_bytes ? bytes_to_gb(crit) : div_by_100 ? cast::to_double(crit) / 100 : cast::to_double(crit);
            if (ImGuiEx::SliderDoubleSmart(crit_name, &val, min, max, format))
                crit = convert_bytes ? gb_to_bytes(val) : div_by_100 ? cast::to_i64(val * 100) : cast::to_i64(val);
            changed.second = ImGui::IsItemDeactivatedAfterEdit();

            ImGui::EndTable();
        }

        return changed;
    };

    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::opt_subtitle_sep));
    ImGui::TextUnformatted("This PC");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), "Render Time Thresholds (Latency)");
    auto changed = show_thresholds("##Latencytable", "Latency##Warning", "Latency##Critical", buffer_i[buff_fps_w], buffer_i[buff_fps_c], false, true, 0.01f, 20.0f, "%.2f ms");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::local_fps, buffer_i[buff_fps_w]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::local_fps, buffer_i[buff_fps_c]);

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), "RAM thresholds");
    changed = show_thresholds("##lramtable", "GB##LRAMWarning", "GB##LRAMCritical", buffer_i[buff_lram_w], buffer_i[buff_lram_c], true, false, 0.5f, 10.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::local_ram, buffer_i[buff_lram_w]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::local_ram, buffer_i[buff_lram_c]);

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), "Disks thresholds");
#ifdef _WIN32
    bool first = true;
#endif
    for (auto& disk : g::pOptsManager->stat_disks_enabled_load())
    {
#ifdef _WIN32
        if (!first)
            ImGui::SameLine();
        first = false;
#endif

        if (ImGui::Checkbox(disk.first.c_str(), &disk.second))
        {
            g::pOptsManager->stat_disks_enabled_save();
        }

    }
    changed = show_thresholds("##diskstable", "GB##DiskWarning", "GB##DiskCritical", buffer_i[buff_disk_w], buffer_i[buff_disk_c], true, false, 1.0f, 40.0f, "%.0f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::local_disk, buffer_i[buff_disk_w]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::local_disk, buffer_i[buff_disk_c]);

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::opt_subtitle_sep));
    ImGui::TextUnformatted("Server");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), "RAM thresholds");
    changed = show_thresholds("##sramtable", "GB##SRAMWarning", "GB##SRAMCritical", buffer_i[buff_sram_w], buffer_i[buff_sram_c], true, false, 1.0f, 10.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::server_ram, buffer_i[buff_sram_w]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::server_ram, buffer_i[buff_sram_c]);

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), "VRAM thresholds");
    changed = show_thresholds("##vramtable", "GB##VRAMWarning", "GB##VRAMCritical", buffer_i[buff_vram_w], buffer_i[buff_vram_c], true, false, 0.1f, 5.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::server_vram, buffer_i[buff_vram_w]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::server_vram, buffer_i[buff_vram_c]);

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), "PyTorch VRAM thresholds");
    changed = show_thresholds("##PyTorchtable", "GB##PyTorchWarning", "GB##PyTorchCritical", buffer_i[buff_pytorch_w], buffer_i[buff_pytorch_c], true, false, 0.1f, 5.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::server_pytorch, buffer_i[buff_pytorch_w]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::server_pytorch, buffer_i[buff_pytorch_c]);

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::opt_separator), "Ping thresholds");
    changed = show_thresholds("##Pingtable", "ms##pingWarning", "GB##PingCritical", buffer_i[buff_ping_w], buffer_i[buff_ping_c], false, false, 50.0f, 500.0f, "%.0f ms");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::server_ping, buffer_i[buff_ping_w]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::server_ping, buffer_i[buff_ping_c]);
}

} // namespace ui
