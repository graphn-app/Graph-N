
#include "ui/options/opt_stats_dlg.h"
#include "core/managers/settings_manager.h"

namespace ui
{

enum
{
    BuffFpsW,
    BuffFpsC,
    BuffLRamW,
    BuffLRamC,
    BuffSRamW,
    BuffSRamC,
    BuffVRamW,
    BuffVRamC,
    BuffDiskW,
    BuffDiskC,
    BuffPyTorchW,
    BuffPyTorchC,
    BuffPingW,
    BuffPingC,

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
    (*data.stats.buffer_i)[BuffFpsW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalFPS);
    (*data.stats.buffer_i)[BuffFpsC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalFPS);
    (*data.stats.buffer_i)[BuffLRamW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalRAM);
    (*data.stats.buffer_i)[BuffLRamC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalRAM);
    (*data.stats.buffer_i)[BuffSRamW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerRAM);
    (*data.stats.buffer_i)[BuffSRamC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerRAM);
    (*data.stats.buffer_i)[BuffVRamW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerVRAM);
    (*data.stats.buffer_i)[BuffVRamC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerVRAM);
    (*data.stats.buffer_i)[BuffDiskW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::LocalDisk);
    (*data.stats.buffer_i)[BuffDiskC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::LocalDisk);
    (*data.stats.buffer_i)[BuffPyTorchW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerPyTorch);
    (*data.stats.buffer_i)[BuffPyTorchC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerPyTorch);
    (*data.stats.buffer_i)[BuffPingW] = g::pOptsManager->stat_threshold_warn_load(MetricLimit::ServerPing);
    (*data.stats.buffer_i)[BuffPingC] = g::pOptsManager->stat_threshold_crit_load(MetricLimit::ServerPing);
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

    const auto show_thresholds = [&bytes_to_gb, &gb_to_bytes, &data](
        const char* warn_name, const char* crit_name,
        int64_t& warn, int64_t& crit, bool convert_bytes, bool div_by_100,
        float min, float max, const char* format)
    {
        std::pair<bool, bool> changed = std::make_pair(false, false);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Warning/Critical:");

        const float avail_x = ImGui::GetContentRegionAvail().x - data.input_offset_x - data.help_mark_w;
        const float slider_w = avail_x / 2;

        ImGui::SameLine();
        ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);

        ImGui::SetNextItemWidth(slider_w);
        double val = convert_bytes ? bytes_to_gb(warn) : div_by_100 ? cast::to_double(warn) / 100 : cast::to_double(warn);
        if (ImGuiEx::SliderDoubleSmart(warn_name, &val, min, max, format))
            warn = convert_bytes ? gb_to_bytes(val) : div_by_100 ? cast::to_i64(val * 100) : cast::to_i64(val);
        changed.first = ImGui::IsItemDeactivatedAfterEdit();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_w);
        val = convert_bytes ? bytes_to_gb(crit) : div_by_100 ? cast::to_double(crit) / 100 : cast::to_double(crit);
        if (ImGuiEx::SliderDoubleSmart(crit_name, &val, min, max, format))
            crit = convert_bytes ? gb_to_bytes(val) : div_by_100 ? cast::to_i64(val * 100) : cast::to_i64(val);
        changed.second = ImGui::IsItemDeactivatedAfterEdit();

        return changed;
    };

    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::OptSubtitleSep));
    ImGui::TextUnformatted("This PC");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Render Time Thresholds (Latency)");
    auto changed = show_thresholds("##LatencyWarning", "##LatencyCritical", buffer_i[BuffFpsW], buffer_i[BuffFpsC], false, true, 0.01f, 20.0f, "%.2f ms");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::LocalFPS, buffer_i[BuffFpsW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::LocalFPS, buffer_i[BuffFpsC]);

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "RAM Thresholds");
    changed = show_thresholds("##LRAMWarning", "##LRAMCritical", buffer_i[BuffLRamW], buffer_i[BuffLRamC], true, false, 0.5f, 10.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::LocalRAM, buffer_i[BuffLRamW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::LocalRAM, buffer_i[BuffLRamC]);

    const auto old_pos = ImGui::GetCursorPosX();
    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Disks Thresholds");

    int32_t disks = 1;
    const float avail_x = ImGui::GetContentRegionAvail().x - data.input_offset_x - data.help_mark_w;
    const float disk_w = avail_x / 2;

    ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
    for (auto& disk : g::pOptsManager->stat_disks_enabled_load())
    {
        if (2 == disks)
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w + disk_w + ImGui::GetStyle().ItemSpacing.x);
        }

        if (ImGui::Checkbox(disk.first.c_str(), &disk.second))
        {
            g::pOptsManager->stat_disks_enabled_save();
        }

        if (++disks > 2)
        {
            disks = 1;
            ImGui::SetCursorPosX(data.input_offset_x + data.help_mark_w);
        }
    }

    ImGui::SetCursorPosX(old_pos);
    changed = show_thresholds("##DiskWarning", "##DiskCritical", buffer_i[BuffDiskW], buffer_i[BuffDiskC], true, false, 1.0f, 40.0f, "%.0f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::LocalDisk, buffer_i[BuffDiskW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::LocalDisk, buffer_i[BuffDiskC]);

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::OptSubtitleSep));
    ImGui::TextUnformatted("Server");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "RAM Thresholds");
    changed = show_thresholds("##SRAMWarning", "##SRAMCritical", buffer_i[BuffSRamW], buffer_i[BuffSRamC], true, false, 1.0f, 10.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::ServerRAM, buffer_i[BuffSRamW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::ServerRAM, buffer_i[BuffSRamC]);

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "VRAM Thresholds");
    changed = show_thresholds("##VRAMWarning", "##VRAMCritical", buffer_i[BuffVRamW], buffer_i[BuffVRamC], true, false, 0.1f, 5.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::ServerVRAM, buffer_i[BuffVRamW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::ServerVRAM, buffer_i[BuffVRamC]);

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "PyTorch VRAM Thresholds");
    changed = show_thresholds("##PyTorchWarning", "##PyTorchCritical", buffer_i[BuffPyTorchW], buffer_i[BuffPyTorchC], true, false, 0.1f, 5.0f, "%.1f GB");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::ServerPyTorch, buffer_i[BuffPyTorchW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::ServerPyTorch, buffer_i[BuffPyTorchC]);

    ImGuiEx::Separator(g::pOptsManager->color_load(AppColor::OptSeparator), "Ping Thresholds");
    changed = show_thresholds("##pingWarning", "##PingCritical", buffer_i[BuffPingW], buffer_i[BuffPingC], false, false, 50.0f, 500.0f, "%.0f ms");
    if (changed.first) g::pOptsManager->stat_threshold_warn_save(MetricLimit::ServerPing, buffer_i[BuffPingW]);
    if (changed.second) g::pOptsManager->stat_threshold_crit_save(MetricLimit::ServerPing, buffer_i[BuffPingC]);
}

} // namespace ui
