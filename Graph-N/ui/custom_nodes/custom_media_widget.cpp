/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_media_widget.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/host_manager.h"
#include "core/base/comfyui_web_paths.h"
#include "ui/graph-n_flow.h"

namespace ui
{

CustomMediaWidget::CustomMediaWidget() noexcept
{
    m_images_in.media_type = m_images_out.media_type = NodeObjFlags::CustomMediaImage;
    m_videos_in.media_type = m_videos_out.media_type = NodeObjFlags::CustomMediaVideo;
    m_audios_in.media_type = m_audios_out.media_type = NodeObjFlags::CustomMediaAudio;
}

CustomMediaWidget::~CustomMediaWidget() noexcept = default;

void CustomMediaWidget::draw_custom_widget(data::NodeInstance* node, float pos, float width)
{
    if (width <= 0.0f) [[unlikely]] return;
    LogRAssert(node, "Zero ptr", );

    m_node = node;
    m_start_pos = pos;
    m_widget_width = width;

    const auto flags = m_node->obj_flags();

    if (flags.has(NodeObjFlags::CustomMediaFromOutput))
    {
        const auto it = m_outputs.find(std::make_pair(node->get_workflow_id(), node->get_instance_id()));
        if (it != m_outputs.end())
        {
            auto& out = it->second;
            if (flags.has(NodeObjFlags::CustomMediaImage) && NodeObjFlags::CustomMediaImage == out.media_type)
                draw_image_widget_from_output(out);
            else if (flags.has(NodeObjFlags::CustomMediaVideo) && NodeObjFlags::CustomMediaVideo == out.media_type)
                draw_video_widget_from_output(out);
            else if (flags.has(NodeObjFlags::CustomMediaAudio) && NodeObjFlags::CustomMediaAudio == out.media_type)
                draw_audio_widget_from_output(out);
            else if (flags.has(NodeObjFlags::CustomMediaText) && NodeObjFlags::CustomMediaText == out.media_type)
                draw_text_widget_from_output(out);
            else
                LogError("Unhandled output widget");
        }
    }
    else
    {
        if (flags.has_all(NodeObjFlags::CustomMediaUploadButton, NodeObjFlags::CustomMediaFromSourceIn))
            draw_widget_upload_button();

        if (flags.has(NodeObjFlags::CustomMediaRefreshButton))
            draw_widget_refresh_button();

        if (flags.has_any(NodeObjFlags::CustomMediaFromSourceIn, NodeObjFlags::CustomMediaFromSourceOut) && nullptr == m_node->get_object()->custom_widget_source) [[unlikely]]
        {
            constexpr size_t err_count = 2;
            constexpr const char* err_lines[err_count] = { "Input source not set", "Please select media source in Node config" };
            draw_widget_message(err_lines, err_count);
        }
        else
        {
            if (flags.has(NodeObjFlags::CustomMediaImage) && flags.has_any(NodeObjFlags::CustomMediaFromSourceIn, NodeObjFlags::CustomMediaFromSourceOut))
                draw_image_widget_from_source();
            else if (flags.has(NodeObjFlags::CustomMediaVideo) && flags.has_any(NodeObjFlags::CustomMediaFromSourceIn, NodeObjFlags::CustomMediaFromSourceOut))
                draw_video_widget_from_source();
            else if (flags.has(NodeObjFlags::CustomMediaAudio) && flags.has_any(NodeObjFlags::CustomMediaFromSourceIn, NodeObjFlags::CustomMediaFromSourceOut))
                draw_audio_widget_from_source();
        }
    }

    m_node = nullptr;
}

void CustomMediaWidget::upload_edited_image(data::NodeInstance* node, const std::filesystem::path& image_path, bool image_is_mask, const std::function<void(const std::string&, const std::string&)>& callback /*= nullptr*/)
{
    m_images_in.upload_target = 0;
    if (node != nullptr)
        m_images_in.upload_target = (node->obj_flags().has(NodeObjFlags::CustomMediaFromSourceIn) ? node->get_instance_id() : 0);
    upload_media(m_images_in, image_path, image_is_mask, image_is_mask, callback);
}

void CustomMediaWidget::draw_widget_message(const char* const lines[], size_t count)
{
    const auto text_centered = [avail = m_widget_width, pos = m_start_pos](const char *text) noexcept
    {
        const float w = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX(pos + (avail - w) * 0.5f);
        ImGui::TextUnformatted(text);
    };

    ImGui::Dummy(ImVec2(10.0f, 20.0f));
    for (size_t i = 0; i < count; i++)
    {
        text_centered(lines[i]);
    }
    ImGui::Dummy(ImVec2(10.0f, 20.0f));
}

void CustomMediaWidget::draw_load_result_message(int32_t load_result)
{
    constexpr size_t err__1_count = 1;
    constexpr const char* err__1[err__1_count] = { "Network error" };                                   // HTTP_RESP_FAIL

    constexpr size_t err__2_count = 1;
    constexpr const char* err__2[err__2_count] = { "Loading..." };                                      // HTTP_RESP_IN_PROGRESS

    constexpr size_t err__3_count = 2;
    constexpr const char* err__3[err__3_count] = { "Media not supported", "or file is broken"};         // media not supported

    constexpr size_t err_404_count = 2;
    constexpr const char* err_404[err_404_count] = { "Media not found", "File deleted or missing" };    // HTTP_RESP_NOT_FOUND

    constexpr size_t err_408_count = 2;
    constexpr const char* err_408[err_408_count] = { "Request timed out", "Server is unresponsive" };   // HTTP_RESP_TIMEOUT

    constexpr size_t err_400_count = 2;
    constexpr const char* err_400[err_400_count] = { "Bad request", "Invalid data format" };            // HTTP_RESP_BAD_REQUEST

    constexpr size_t err_405_count = 2;
    constexpr const char* err_405[err_405_count] = { "Method not allowed", "API endpoint mismatch" };   // HTTP_RESP_NOT_ALLOWED

    constexpr size_t err_def_count = 1;
    constexpr const char* err_def[err_def_count] = { "Unknown error" };                                 // other

    switch (load_result)
    {
    case HTTP_RESP_FAIL:
        draw_widget_message(err__1, err__1_count);
        break;
    case HTTP_RESP_IN_PROGRESS:
        draw_widget_message(err__2, err__2_count);
        break;
    case -3:
        draw_widget_message(err__3, err__3_count);
        break;
    case HTTP_RESP_TIMEOUT:
        draw_widget_message(err_408, err_408_count);
        break;
    case HTTP_RESP_BAD_REQUEST:
        draw_widget_message(err_400, err_400_count);
        break;
    case HTTP_RESP_NOT_FOUND:
        draw_widget_message(err_404, err_404_count);
        break;
    case HTTP_RESP_NOT_ALLOWED:
        draw_widget_message(err_405, err_405_count);
        break;
    default:
        draw_widget_message(err_def, err_def_count);
        break;
    }
}

void CustomMediaWidget::check_media_list_version(Widget& widget)
{
    if (-1 == widget.list_version) [[unlikely]] // init global list
    {
        const auto& pin_list = m_node->get_object()->custom_widget_source->combo_items;
        widget.names.clear();
        std::ranges::for_each(pin_list, [&widget](const Primitive& item) { widget.names.push_back(item.get_string()); });
        widget.list_version = 0;
        widget.reset = false;
        return;
    }

    if (widget.list_version != 0) // reset
        LogRAssert(m_node->m_media_list_version <= widget.list_version, "Local list version > global list version", );

    if (m_node->m_media_list_version == widget.list_version) [[likely]]
        return;
    m_node->m_media_list_version = widget.list_version;

    if (widget.reset)
    {
        const auto pin = m_node->get_object()->custom_widget_source;
        pin->combo_items.clear();
        for (auto& item : widget.names)
        {
            pin->combo_items.push_back(item);
        }
        pin->finalize_combo();
        return;
    }

    if ((0 == widget.upload_target || widget.upload_target == m_node->get_instance_id()) && !widget.uploaded_media.empty())
    {
        const auto pin = m_node->get_object()->custom_widget_source;
        for (auto& input : m_node->get_inputs())
        {
            if (input->get_object()->get_hash() == pin->get_hash())
            {
                auto& pin_list = pin->combo_items;
                pin_list.push_back(widget.uploaded_media);
                pin->finalize_combo();
                if (widget.upload_target != 0)
                    input->state().value = cast::to_i64(pin_list.size() - 1);
                break;
            }
        }

        widget.uploaded_media.clear();
        widget.upload_target = 0;
    }
}

bool CustomMediaWidget::check_source_and_get_pin(Widget& widget, int64_t& index, data::Pin*& source_pin, bool skip_media_validation /*= false*/)
{
    source_pin = m_node->get_object()->custom_widget_source;
    if (nullptr == source_pin)
        return false;

    check_media_list_version(widget);

    if (source_pin->combo_items_str.empty())
        return false;

    if (widget.items.size() < widget.names.size())
        widget.items.resize(widget.names.size());

    data::PinInstance* source_pin_inst = nullptr;
    for (auto& input : m_node->get_inputs())
    {
        if (input->get_object()->get_hash() == source_pin->get_hash())
        {
            source_pin_inst = input;
            break;
        }
    }
    LogRAssert(source_pin_inst, "Source pin instance not found. w: " << m_node->get_workflow_id() << ", n:" << m_node->get_instance_id(), false);

    index = source_pin_inst->state().value.get_int();
    const auto& str = source_pin->combo_items_str[index];

    if (cast::to_szt(index) >= widget.names.size() || widget.names[index] != str)
    {
        index = -1;
        for (size_t idx = 0; idx < widget.names.size(); idx++)
        {
            if (widget.names[idx] == str)
            {
                index = cast::to_i64(idx);
                break;
            }
        }
    }
    if (-1 == index)
    {
        widget.items.push_back({});
        widget.names.push_back(str);
        index = cast::to_i64(widget.names.size() - 1);
    }

    if (skip_media_validation) [[unlikely]]
        return true;

    if (!widget.items[index].media_valid.value_or(true) || !widget.items[index].media_can_play.value_or(true))
    {
        widget.items[index].load_result = -3;
        draw_load_result_message(widget.items[index].load_result);
        return false;
    }

    return true;
}

void CustomMediaWidget::download_media(HttpRequestor requestor, Widget& widget, int64_t index, bool from_output)
{
    auto& widget_item = widget.items[index];

    if (widget_item.requested)
    {
        if (widget_item.load_result != HTTP_RESP_OK)
            draw_load_result_message(widget_item.load_result);
        return;
    }

    std::filesystem::path source_name = widget.names[index];
    const auto callback = [&widget, w_id = m_node->get_workflow_id(), n_id = m_node->get_instance_id(), index, ext = source_name.extension().string()]
    (int32_t result, std::string&& data)
    {
        LogRAssertS(cast::to_szt(index) < widget.items.size(), "Index out of bound. w: " << w_id << ", n:" << n_id << ", idx:" << index << ", lst:" << widget.items.size(), );

        auto& widget_item = widget.items[index];
        widget_item.load_result = result;
        if (result != HTTP_RESP_OK || data.empty())
            return;

        auto temp_filename = fs::get_temp_filepath(ext);
        if (!temp_filename.empty())
        {
            if (tools::path::make_file(temp_filename, true, data))
            {
                widget_item.temp_path = std::move(temp_filename);
                return;
            }
        }

        result = HTTP_RESP_FAIL;
    };

    const std::string directory = tools::path::path_to_string(source_name.parent_path());
    const std::string filename = tools::path::path_to_string(source_name.filename()) + (from_output ? " [output]" : "");
    const auto request = fmt::format(comfyui_http::view_input_, tools::str::url_encode(filename), tools::str::url_encode(directory));
    g::pFlow->http_get(requestor, get_alive_callback(), request.c_str(), callback, true);
    widget_item.requested = true;
    widget_item.load_result = HTTP_RESP_IN_PROGRESS;
}

const char* CustomMediaWidget::format_timeline(float duration, float pos)
{
    const auto get_times = [](const float time, int32_t& h, int32_t& m, int32_t& s, int32_t& ms) noexcept
    {
        s = cast::to_i32(time);
        ms = cast::to_i32((time - s) * 1000.0f);
        h = s / 3600;
        m = (s % 3600) / 60;
        s = s % 60;
    };

    int32_t total_h, total_m, total_s, total_ms;
    get_times(duration, total_h, total_m, total_s, total_ms);

    int32_t curr_h, curr_m, curr_s, curr_ms;
    get_times(pos, curr_h, curr_m, curr_s, curr_ms);

    static char buffer[100] = {};
    if (total_h > 0)
        *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING("{:02}:{:02}:{:02}.{:03}"), curr_h, curr_m, curr_s, curr_ms).out = 0;
    else if (total_m > 0)
        *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING("{:02}:{:02}.{:03}"), curr_m, curr_s, curr_ms).out = 0;
    else
        *fmt::format_to_n(buffer, sizeof(buffer), FMT_STRING("{:02}.{:03}"), curr_s, curr_ms).out = 0;

    return buffer;
}

void CustomMediaWidget::reset_widgets() noexcept
{
    m_images_in = {};
    m_images_out = {};
    m_videos_in = {};
    m_videos_out = {};
    m_audios_in = {};
    m_audios_out = {};
}

void CustomMediaWidget::add_output(
    wid wf_id, iid node_id, mngr::ExecTask::OutputType type, const std::string& data, const std::string& server_filename, int32_t preloaded_media_id)
{
    const auto* wf = g::pWfsManager->access(wf_id, true);
    if (nullptr == wf)   // zero wf is not an error, it could be closed already
        return;
    auto node = wf->get_node_by_instance(node_id);
    if (nullptr == node)   // zero node is not an error, it could be removed
        return;

    OutputData out;
    out.item.media_id = preloaded_media_id;
    switch (type)
    {
    case mngr::ExecTask::OutputType::Audio:
        out.media_type = NodeObjFlags::CustomMediaAudio;
        out.item.temp_path = data;
        break;
    case mngr::ExecTask::OutputType::Gif:
        out.media_type = NodeObjFlags::CustomMediaVideo;
        out.item.temp_path = data;
        break;
    case mngr::ExecTask::OutputType::Image:
        out.media_type = NodeObjFlags::CustomMediaImage;
        out.item.temp_path = data;
        break;
    case mngr::ExecTask::OutputType::Text:
        out.media_type = NodeObjFlags::CustomMediaText;
        out.item.text_info = data;
        break;
    default:
        LogRError("Unknwon output type", );
    }

    out.item.server_name = server_filename;
    node->obj_flags().remove(NodeObjFlags::CustomMediaFromSourceIn, NodeObjFlags::CustomMediaFromSourceOut, NodeObjFlags::CustomMediaRefreshButton, NodeObjFlags::CustomMediaUploadButton, NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaText);
    node->obj_flags().add(NodeObjFlags::RestrictCfg, NodeObjFlags::CustomMediaWidget, NodeObjFlags::CustomMediaFromOutput, out.media_type);
    node->reset_width();

    m_outputs[std::make_pair(wf_id, node_id)] = std::move(out);
}

void CustomMediaWidget::cleanup_outputs(wid wf_id)
{
    std::erase_if(m_outputs, [wf_id]
        (const std::pair<std::pair<wid, iid>, OutputData>& value)
        {
            return value.first.first == wf_id;
        }
    );
}

void CustomMediaWidget::cleanup_outputs(wid wf_id, iid node_id)
{
    std::erase_if(m_outputs, [wf_id, node_id]
        (const std::pair<std::pair<wid, iid>, OutputData>& value)
        {
            return value.first.first == wf_id && value.first.second == node_id;
        }
    );
}

bool CustomMediaWidget::check_open_def_app(data::NodeInstance* node, bool check_only)
{
    const auto flags = node->obj_flags();
    if (flags.has_not(NodeObjFlags::CustomMediaWidget))
        return false;

    if (flags.has(NodeObjFlags::CustomMediaFromOutput))
    {
        const auto it = m_outputs.find(std::make_pair(node->get_workflow_id(), node->get_instance_id()));
        if (it != m_outputs.end())
        {
            auto& out = it->second;
            if (check_only)
                return !out.item.temp_path.empty();
            return tools::path::open_file(out.item.temp_path, false);
        }
        return false;
    }

    if (flags.has_any(NodeObjFlags::CustomMediaFromSourceIn, NodeObjFlags::CustomMediaFromSourceOut) && flags.has_any(NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaAudio))
    {
        int64_t index = 0;
        data::Pin* source_pin_obj = nullptr;
        Widget* widget = nullptr;

        if (flags.has_all(NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaFromSourceIn))
            widget = &m_images_in;
        else if (flags.has_all(NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaFromSourceOut))
            widget = &m_images_out;
        else if (flags.has_all(NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaFromSourceIn))
            widget = &m_videos_in;
        else if (flags.has_all(NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaFromSourceOut))
            widget = &m_videos_out;
        else if (flags.has_all(NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaFromSourceIn))
            widget = &m_audios_in;
        else if (flags.has_all(NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaFromSourceOut))
            widget = &m_audios_out;
        else
            return false;

        m_node = node;
        const bool result = check_source_and_get_pin(*widget, index, source_pin_obj, true);
        m_node = nullptr;
        if (!result)
            return false;

        if (check_only)
            return !widget->items[index].temp_path.empty();
        return tools::path::open_file(widget->items[index].temp_path, false);
    }

    return false;
}

void CustomMediaWidget::insert_base64(WidgetItem& widget)
{
    if (nullptr == m_node->get_object()->base64_widget_dest || !tools::path::exists(widget.temp_path))
        return;

    if (!widget.base64)
    {
        widget.base64 = std::make_shared<Base64>();
        widget.base64->ready.store(false, std::memory_order_relaxed);

        std::thread([filepath = widget.temp_path, base64_weak = std::weak_ptr<Base64>(widget.base64)]() 
        {
            const auto data = tools::path::read_file(filepath, false);
            auto str = tools::str::base64_encode(data, "data:" + tools::img::get_mime_type(filepath) + ";base64,");
            auto ptr = base64_weak.lock();
            if (ptr)
            {
                ptr->base64 = std::move(str);
                ptr->ready.store(true, std::memory_order_release);
            }
        }).detach();

        return;
    }

    if (!widget.base64->ready.load(std::memory_order_acquire))
        return;

    for (auto pin : m_node->get_inputs())
    {
        if (pin->get_object() == m_node->get_object()->base64_widget_dest)
        {
            *pin->state().value.get_string_ref() = widget.base64->base64;
            pin->obj_flags().add(PinObjFlags::Base64Inside, PinObjFlags::DenyMultilineSwitch);
            pin->flags().remove(PinInstFlags::Multiline, PinInstFlags::MultilineSet);
            widget.base64_inserted = true;
            Log("Base64 calculated and inserted into w:" << m_node->get_workflow_id() << ", n:" << m_node->get_instance_id() << ", p:" << pin->get_instance_id());
            break;
        }
    }
}

void CustomMediaWidget::reinsert_base64(Widget& widget, size_t curr_index)
{
    if (widget.items[curr_index].base64_inserted &&
        size_t(widget.base64_last_index) != curr_index &&
        widget.items[curr_index].base64 &&
        !widget.items[curr_index].base64->base64.empty())
    {
        widget.base64_last_index = cast::to_i64(curr_index);

        for (auto pin : m_node->get_inputs())
        {
            if (pin->get_object() == m_node->get_object()->base64_widget_dest)
            {
                *pin->state().value.get_string_ref() =  widget.items[curr_index].base64->base64;
                pin->obj_flags().add(PinObjFlags::Base64Inside, PinObjFlags::DenyMultilineSwitch);
                pin->flags().remove(PinInstFlags::Multiline, PinInstFlags::MultilineSet);
                break;
            }
        }
    }
}

} // namespace ui
