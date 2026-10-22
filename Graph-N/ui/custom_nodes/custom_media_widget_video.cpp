/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_media_widget.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/settings_manager.h"
#include "ui/helpers/video_control.h"
#include "ui/helpers/textures.h"
#include "main.h"

namespace ui
{
void CustomMediaWidget::draw_video(WidgetItem& widget)
{
    bool is_playing = false;
    float video_pos = 0.0f;

    if (-1 == widget.media_id || g::pVideoControl->get_current() != widget.media_id)
    {
        if (!g::pTextures->get_image_alive(widget.video_info.thumbnail_id))
        {
            widget.video_info.thumbnail_id = g::pTextures->load_image_from_path(widget.video_info.thumbnail.c_str(), NODE_IMAGE_RECT, NODE_IMAGE_RECT, NODE_IMAGE_LIFETIME);

            if (widget.video_info.width > 0 && widget.video_info.height > 0 && widget.or_size.empty())
            {
                if (widget.video_info.width != widget.video_info.height)
                    widget.or_size = fmt::format(FMT_STRING("{} x {} [{}], {:.2f}s, {}fps"), widget.video_info.width, widget.video_info.height, tools::img::get_readable_aspect_ratio(widget.video_info.width, widget.video_info.height), widget.video_info.duration_sec, cast::to_u32(widget.video_info.fps));
                else
                    widget.or_size = fmt::format(FMT_STRING("{} x {}, {:.2f}s, {}fps"), widget.video_info.width, widget.video_info.height, widget.video_info.duration_sec, cast::to_u32(widget.video_info.fps));
                ImGui::PushFont(g::pMain->get_small_font());
                widget.or_size_w = ImGui::CalcTextSize(widget.or_size.c_str()).x;
                ImGui::PopFont();
            }
        }
        else
        {
            const auto image = g::pTextures->get_image(widget.video_info.thumbnail_id, widget.media_valid);
            if (image)
            {
                ImVec2 rect = { m_widget_width, m_widget_width };
                if (widget.video_info.width > m_widget_width && widget.video_info.height > 0 && widget.video_info.width != widget.video_info.height)
                {
                    int32_t res_x, res_y;
                    tools::img::scale(widget.video_info.width, widget.video_info.height, cast::to_i32(rect.x), 0, res_x, res_y);
                    rect.x = cast::to_float(res_x);
                    rect.y = cast::to_float(res_y);
                }

                ImGui::SetCursorPosX(m_start_pos);
                ImGui::Image(image, rect);
            }
        }
    }
    else
    {
        is_playing = g::pVideoControl->get_is_playing(widget.media_id);
        video_pos = g::pVideoControl->get_position(widget.media_id);

        const auto frame = g::pVideoControl->get_frame(widget.media_id);
        if (frame)
        {
            ImVec2 rect = { m_widget_width, m_widget_width };
            if (widget.video_info.width > m_widget_width && widget.video_info.height > 0 && widget.video_info.width != widget.video_info.height)
            {
                int32_t res_x, res_y;
                tools::img::scale(widget.video_info.width, widget.video_info.height, cast::to_i32(rect.x), 0, res_x, res_y);
                rect.x = cast::to_float(res_x);
                rect.y = cast::to_float(res_y);
            }

            ImGui::SetCursorPosX(m_start_pos);
            ImGui::Image(frame, rect);
        }
    }

    const auto load = [&widget]()
    {
        widget.media_id = g::pVideoControl->load(widget.temp_path, NODE_IMAGE_RECT);
        widget.media_can_play = (widget.media_id != 0);
        if (0 == widget.media_id) widget.media_id = -1;
    };

    const bool can_play = widget.media_can_play.value_or(true);
    if (!can_play)
        ImGui::BeginDisabled();

    ImGui::SetCursorPosX(m_start_pos);
    if (ImGui::Button(is_playing ? ICON_LC_SQUARE : ICON_LC_PLAY, ImVec2(30, 0))) [[unlikely]]
    {
        if (is_playing)
            g::pVideoControl->stop(widget.media_id);
        else
        {
            if (g::pVideoControl->get_current() != widget.media_id)
                load();
            if (widget.media_can_play.value_or(false))
                g::pVideoControl->play(widget.media_id);
        }
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(m_widget_width - (ImGui::GetCursorPosX() - m_start_pos));
    if (ImGui::SliderFloat("##video_timeline", &video_pos, 0.0f, widget.video_info.duration_sec, format_timeline(widget.video_info.duration_sec, video_pos)))
    {
        if (g::pVideoControl->get_current() != widget.media_id)
            load();
        if (widget.media_can_play.value_or(false))
            g::pVideoControl->seek(widget.media_id, video_pos);
    }
    ImGui::PopItemWidth();

    if (!can_play)
        ImGui::EndDisabled();

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


    if (m_node->obj_flags().has(NodeObjFlags::CustomMediaBase64) && !widget.base64_inserted)
    {
        insert_base64(widget);
    }
}

void CustomMediaWidget::draw_video_widget_from_output(OutputData& out)
{
    if (!out.item.media_valid.has_value())
    {
        out.item.or_size.clear();
        out.item.video_info = g::pVideoControl->get_info(out.item.temp_path, NODE_IMAGE_RECT, out.item.media_valid);
        if (!out.item.media_valid.value() || out.item.video_info.thumbnail.empty())
        {
            out.item.media_valid = false;
            return;
        }
        out.item.media_id = -1;
    }
    if (!out.item.media_valid.value() || !out.item.media_can_play.value_or(true))
    {
        out.item.load_result = -3;
        draw_load_result_message(out.item.load_result);
        return;
    }

    draw_video(out.item);
}

void CustomMediaWidget::draw_video_widget_from_source()
{
    auto data_widget = m_node->obj_flags().has(NodeObjFlags::CustomMediaFromSourceIn) ? &m_videos_in : &m_videos_out;

    int64_t index = 0;
    data::Pin* source_pin_obj = nullptr;
    if (!check_source_and_get_pin(*data_widget, index, source_pin_obj))
        return;

    auto& widget = data_widget->items[index];

    if (widget.media_valid.value_or(false))
    {
        draw_video(widget);

        if (m_node->obj_flags().has(NodeObjFlags::CustomMediaBase64) && widget.base64_inserted)
        {
            reinsert_base64(*data_widget, index);
        }
    }
    else if (!widget.temp_path.empty())
    {
        widget.or_size.clear();
        widget.video_info = g::pVideoControl->get_info(widget.temp_path, NODE_IMAGE_RECT, widget.media_valid);
        if (!widget.media_valid.value() || widget.temp_path.empty())
        {
            widget.media_valid = false;
            return;
        }
        widget.media_id = -1;
    }
    else
        download_media(HttpRequestor::CustomWdgt_Video, *data_widget, index, m_node->obj_flags().has(NodeObjFlags::CustomMediaFromSourceOut));
}

} // namespace ui
