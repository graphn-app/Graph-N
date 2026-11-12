/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_media_widget.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/settings_manager.h"
#include "ui/helpers/textures.h"
#include "main.h"

namespace ui
{

void CustomMediaWidget::draw_image(WidgetItem& widget)
{
    if (g::pTextures->get_image_alive(widget.media_id))
    {
        float w = 0.0f, h = 0.0f;
        const auto textures = g::pTextures->get_image(widget.media_id, widget.media_valid, &w, &h);
        if (textures && widget.media_valid.value_or(false))
        {
            int32_t res_w = 0, res_h = 0;
            tools::img::scale(cast::to_i32(w), cast::to_i32(h), cast::to_i32(m_widget_width), 0, res_w, res_h);
            float offset = (m_widget_width / 2) - (cast::to_float(res_w) / 2) + m_start_pos;
            ImGui::SetCursorPosX(offset);
            ImGui::Image(textures, ImVec2(cast::to_float(res_w), cast::to_float(res_h)));

            m_node->flags().add(NodeInstFlags::MaskEditorReady, NodeInstFlags::PosEditorReady);

            if (widget.or_size.empty())
            {
                int32_t or_w = 0, or_h = 0;
                g::pTextures->get_original_image_size(widget.media_id, or_w, or_h);
                if (or_w > 0 && or_h > 0)
                {
                    if (or_w != or_h)
                        widget.or_size = fmt::format(FMT_STRING("{} x {} [{}]"), or_w, or_h, tools::img::get_readable_aspect_ratio(or_w, or_h));
                    else
                        widget.or_size = fmt::format(FMT_STRING("{} x {}"), or_w, or_h);
                    ImGui::PushFont(g::pMain->get_small_font());
                    widget.or_size_w = ImGui::CalcTextSize(widget.or_size.c_str()).x;
                    ImGui::PopFont();
                }
            }
            if (!widget.or_size.empty())
            {
                offset = (m_widget_width / 2) - (widget.or_size_w / 2) + m_start_pos;
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
    }
    else
    {
        widget.media_id = g::pTextures->load_image_from_path(widget.temp_path, NODE_IMAGE_RECT, NODE_IMAGE_RECT, NODE_IMAGE_LIFETIME);
        widget.media_valid = (widget.media_id != 0);
    }

    if (m_node->obj_flags().has(NodeObjFlags::CustomMediaBase64) && !widget.base64_inserted)
    {
        insert_base64(widget);
    }
}

void CustomMediaWidget::draw_image_widget_from_output(OutputData& out)
{
    if (!out.item.media_valid.has_value())
        out.item.media_valid = tools::path::exists(out.item.temp_path);
    if (!out.item.media_valid.value())
    {
        out.item.load_result = -3;
        draw_load_result_message(out.item.load_result);
        return;
    }

    draw_image(out.item);
}

void CustomMediaWidget::draw_image_widget_from_source()
{
    m_node->flags().remove(NodeInstFlags::MaskEditorReady, NodeInstFlags::PosEditorReady);

    auto data_widget = m_node->obj_flags().has(NodeObjFlags::CustomMediaFromSourceIn) ? &m_images_in : &m_images_out;

    int64_t index = 0;
    data::Pin* source_pin_obj = nullptr;
    if (!check_source_and_get_pin(*data_widget, index, source_pin_obj))
        return;

    auto& widget = data_widget->items[index];

    if (widget.media_valid.value_or(false))
    {
        draw_image(widget);

        if (m_node->obj_flags().has(NodeObjFlags::CustomMediaBase64) && widget.base64_inserted)
        {
            reinsert_base64(*data_widget, index);
        }
    }
    else if (!widget.temp_path.empty())
    {
        widget.or_size.clear();
        widget.media_id = g::pTextures->load_image_from_path(widget.temp_path, NODE_IMAGE_RECT, NODE_IMAGE_RECT, NODE_IMAGE_LIFETIME);
        widget.media_valid = (widget.media_id != 0);
    }
    else
        download_media(HttpRequestor::CustomWdgt_Image, *data_widget, index, m_node->obj_flags().has(NodeObjFlags::CustomMediaFromSourceOut));
}

bool CustomMediaWidget::get_current_image_paths(data::NodeInstance* node, std::filesystem::path& path, std::filesystem::path& server_name) const
{
    if (node->obj_flags().has(NodeObjFlags::CustomMediaImage) && node->obj_flags().has_any(NodeObjFlags::CustomMediaFromSourceIn, NodeObjFlags::CustomMediaFromSourceOut))
    {
        auto source_pin_obj = node->get_object()->custom_widget_source;
        if (nullptr == source_pin_obj)
            return false;
        data::PinInstance* source_pin_inst = nullptr;
        for (auto& input : node->get_inputs())
        {
            if (input->get_object()->get_hash() == source_pin_obj->get_hash())
            {
                source_pin_inst = input;
                break;
            }
        }
        LogRAssert(source_pin_inst, "Source pin instance not found. w: " << node->get_workflow_id() << ", n:" << node->get_instance_id(), false);

        const int64_t index = source_pin_inst->state().value.get_int();
        LogRAssert(index >= 0 && cast::to_szt(index) < source_pin_obj->combo_items_ptr.size(), "Indext out of bounds. w: " << node->get_workflow_id() << ", n:" << node->get_instance_id() << ", p:" << source_pin_inst->get_instance_id(), false);

        auto data_widget = node->obj_flags().has(NodeObjFlags::CustomMediaFromSourceIn) ? &m_images_in : &m_images_out;
        if (cast::to_szt(index) >= data_widget->items.size())
            return false;
        if (g::pTextures->get_image_alive(data_widget->items[index].media_id))
        {
            path = data_widget->items[index].temp_path;
            server_name = source_pin_obj->combo_items_ptr[index];
            return tools::path::exists(path);
        }
        return false;
    }
    else if(node->obj_flags().has_all(NodeObjFlags::CustomMediaFromOutput, NodeObjFlags::CustomMediaImage))
    {
        const auto it = m_outputs.find(std::make_pair(node->get_workflow_id(), node->get_instance_id()));
        if (it != m_outputs.end())
        {
            path = it->second.item.temp_path;
            server_name = (it->second.item.server_name.empty() ? path : it->second.item.server_name);
            return tools::path::exists(path);
        }
        return false;
    }

    return false;
}

} // namespace ui
