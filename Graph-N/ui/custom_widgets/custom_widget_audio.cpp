#include "ui/custom_widgets/custom_widget.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/settings_manager.h"
#include "ui/helpers/audio_control.h"
#include "main.h"

namespace ui
{

void CustomWidget::draw_audio(WidgetItem& widget)
{
    const bool is_playing = g::pAudioControl->get_is_playing(widget.media_id);

    const auto load = [&widget]()
    {
        widget.media_id = g::pAudioControl->load(tools::path::path_to_string(widget.temp_path));
        widget.media_valid = (widget.media_id != 0);
    };

    ImGui::SetCursorPosX(m_start_pos);
    if (ImGui::Button(is_playing ? ICON_LC_SQUARE : ICON_LC_PLAY, ImVec2(30, 0))) [[unlikely]]
    {
        if (is_playing)
            g::pAudioControl->stop(widget.media_id);
        else
        {
            if (g::pAudioControl->get_current() != widget.media_id)
                load();
            if (widget.media_valid.value_or(false))
                g::pAudioControl->play(widget.media_id);
        }
    }

    float audio_pos = g::pAudioControl->get_position(widget.media_id);

    ImGui::SameLine();
    ImGui::PushItemWidth(m_widget_width - (ImGui::GetCursorPosX() - m_start_pos));    
    if (ImGui::SliderFloat("##audio_timeline", &audio_pos, 0.0f, widget.audio_info.duration_sec, format_timeline(widget.audio_info.duration_sec, audio_pos)))
    {
        if (g::pAudioControl->get_current() != widget.media_id)
            load();
        if (widget.media_valid.value_or(false))
            g::pAudioControl->seek(widget.media_id, audio_pos);
    }
    ImGui::PopItemWidth();

    if (!widget.or_size.empty())
    {
        const auto offset = (m_widget_width / 2) - (widget.or_size_w / 2) + m_start_pos;
        ImGui::SetCursorPosX(offset);
        ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodePinText));
        ImGui::PushFont(g::pMain->get_small_font());
        ImGui::TextUnformatted(widget.or_size.c_str(), widget.or_size.c_str() + widget.or_size.size());
        ImGui::PopFont();
        ImGui::PopStyleColor();
    }
    else
        ImGui::Dummy(ImVec2(m_widget_width, 10.0f));
}

void CustomWidget::draw_audio_widget_from_output(OutputData& out)
{
    if (!out.item.media_valid.has_value())
    {
        out.item.media_valid = tools::path::exists(out.item.temp_path);
        if (!out.item.media_valid.value())
            return;

        out.item.or_size.clear();
        out.item.audio_info = g::pAudioControl->get_info(tools::path::path_to_string(out.item.temp_path), out.item.media_valid);
        out.item.media_id = -1;

        if (out.item.media_valid.value_or(false))
        {
            out.item.or_size = fmt::format(FMT_STRING("{:.3f}s, {}Hz, {}ch"), out.item.audio_info.duration_sec, out.item.audio_info.sample_rate, out.item.audio_info.channels);
            ImGui::PushFont(g::pMain->get_small_font());
            out.item.or_size_w = ImGui::CalcTextSize(out.item.or_size.c_str()).x;
            ImGui::PopFont();
        }
    }
    if (!out.item.media_valid.value())
    {
        out.item.load_result = -3;
        draw_load_result_message(out.item.load_result);
        return;
    }

    draw_audio(out.item);
}

void CustomWidget::draw_audio_widget_from_source()
{
    int64_t index = 0;
    data::Pin* source_pin_obj = nullptr;
    if (!check_source_and_get_pin(m_audios, index, source_pin_obj))
        return;

    auto& widget = m_audios.items[index];

    if (widget.media_valid.value_or(false))
    {
        draw_audio(widget);
    }
    else if (!widget.temp_path.empty())
    {
        widget.or_size.clear();
        widget.audio_info = g::pAudioControl->get_info(tools::path::path_to_string(widget.temp_path), widget.media_valid);
        widget.media_id = -1;

        if (widget.media_valid.value_or(false))
        {
            widget.or_size = fmt::format(FMT_STRING("{:.3f}s, {}Hz, {}ch"), widget.audio_info.duration_sec, widget.audio_info.sample_rate, widget.audio_info.channels);
            ImGui::PushFont(g::pMain->get_small_font());
            widget.or_size_w = ImGui::CalcTextSize(widget.or_size.c_str()).x;
            ImGui::PopFont();
        }
    }
    else
        download_media(HttpRequestor::CustomWdgt_Audio, m_audios, index);
}

} // namespace ui
