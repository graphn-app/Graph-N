
#include "ui/options/opt_stats_dlg.h"
#include "core/managers/settings_manager.h"

namespace ui
{

void OptStatsDlg::init(OptsData& /*data*/)
{
    LogF;

    g::pOptsManager->stat_get_lram_threshold(m_lim_lram.warning, m_lim_lram.critical);
    g::pOptsManager->stat_get_sram_threshold(m_lim_sram.warning, m_lim_sram.critical);
    g::pOptsManager->stat_get_vram_threshold(m_lim_vram.warning, m_lim_vram.critical);
    g::pOptsManager->stat_get_pytorch_threshold(m_lim_pytorch.warning, m_lim_pytorch.critical);
    g::pOptsManager->stat_get_disk_threshold(m_lim_disk.warning, m_lim_disk.critical);
    g::pOptsManager->stat_get_fps_threshold(m_lim_fps.warning, m_lim_fps.critical);
    g::pOptsManager->stat_get_ping_threshold(m_lim_ping.warning, m_lim_ping.critical);

    m_disks = g::pOptsManager->stat_get_disks_enabled();
}

void OptStatsDlg::show(OptsData& data)
{
    const auto bytes_to_gb = [](int64_t bytes) -> double
    {
        double f = cast::to_double(bytes);
        f /= 1024.0f * 1024.0f * 1024.0f;
        return f;
    };

    const auto gb_to_bytes = [](double gb) -> int64_t
    {
        return cast::to_i64(gb * 1024.0f * 1024.0f * 1024.0f);
    };

    const auto show_thresholds = [&bytes_to_gb, &gb_to_bytes](
        const char* table_name, const char* warn_name, const char* crit_name,
        MetricLimits& lim, bool convert_bytes,
        float min, float max, const char* format)
    {
        bool chnaged = false;
        if (ImGui::BeginTable(table_name, 2, ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Warning:");

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            double val = convert_bytes ? bytes_to_gb(lim.warning) : cast::to_float(lim.warning);
            if (ImGuiEx::SliderDoubleSmart(warn_name, &val, min, max, format))
            {
                lim.warning = convert_bytes ? gb_to_bytes(val) : cast::to_i64(val);
                chnaged = true;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Critical:");

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            val = convert_bytes ? bytes_to_gb(lim.critical) : cast::to_float(lim.critical);
            if (ImGuiEx::SliderDoubleSmart(crit_name, &val, min, max, format))
            {
                lim.critical = convert_bytes ? gb_to_bytes(val) : cast::to_i64(val);
                chnaged = true;
            }

            ImGui::EndTable();
        }

        return chnaged;
    };

    ImGui::PushStyleColor(ImGuiCol_Text, data.caption_separator_clr);
    ImGui::TextUnformatted("This PC");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    ImGuiEx::Separator(data.separator_clr, "FPS thresholds");
    if (show_thresholds("##fpstable", "FPS##Warning", "FPS##Critical", m_lim_fps, false, 5.0f, 60.0f, "%.0f FPS"))
        g::pOptsManager->stat_set_fps_threshold(m_lim_fps.warning, m_lim_fps.critical);

    ImGuiEx::Separator(data.separator_clr, "RAM thresholds");
    if (show_thresholds("##lramtable", "GB##LRAMWarning", "GB##LRAMCritical", m_lim_lram, true, 0.5f, 10.0f, "%.1f GB"))
        g::pOptsManager->stat_set_lram_threshold(m_lim_lram.warning, m_lim_lram.critical);

    ImGuiEx::Separator(data.separator_clr, "Disks thresholds");
    bool first = true;
    for (auto& disk : m_disks)
    {
        if (!first)
            ImGui::SameLine();
        first = false;

        if (ImGui::Checkbox(disk.first.c_str(), &disk.second))
        {
            g::pOptsManager->stat_set_disks_enabled(m_disks);
        }

    }
    if (show_thresholds("##diskstable", "GB##DiskWarning", "GB##DiskCritical", m_lim_disk, true, 1.0f, 40.0f, "%.0f GB"))
        g::pOptsManager->stat_set_disk_threshold(m_lim_disk.warning, m_lim_disk.critical);

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, data.caption_separator_clr);
    ImGui::TextUnformatted("Server");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    ImGuiEx::Separator(data.separator_clr, "RAM thresholds");
    if (show_thresholds("##sramtable", "GB##SRAMWarning", "GB##SRAMCritical", m_lim_sram, true, 1.0f, 10.0f, "%.1f GB"))
        g::pOptsManager->stat_set_sram_threshold(m_lim_sram.warning, m_lim_sram.critical);

    ImGuiEx::Separator(data.separator_clr, "VRAM thresholds");
    if (show_thresholds("##vramtable", "GB##VRAMWarning", "GB##VRAMCritical", m_lim_vram, true, 0.1f, 5.0f, "%.1f GB"))
        g::pOptsManager->stat_set_vram_threshold(m_lim_vram.warning, m_lim_vram.critical);

    ImGuiEx::Separator(data.separator_clr, "PyTorch VRAM thresholds");
    if (show_thresholds("##PyTorchtable", "GB##PyTorchWarning", "GB##PyTorchCritical", m_lim_pytorch, true, 0.1f, 5.0f, "%.1f GB"))
        g::pOptsManager->stat_set_pytorch_threshold(m_lim_pytorch.warning, m_lim_pytorch.critical);

    ImGuiEx::Separator(data.separator_clr, "Ping thresholds");
    if (show_thresholds("##Pingtable", "ms##pingWarning", "GB##PingCritical", m_lim_ping, false, 50.0f, 500.0f, "%.0f ms"))
        g::pOptsManager->stat_set_ping_threshold(m_lim_ping.warning, m_lim_ping.critical);
}

} // namespace ui
