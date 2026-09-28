#include "ui/custom_widgets/custom_widget.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/host_manager.h"
#include "core/base/comfyui_web_paths.h"
#include "ui/graph-n_flow.h"

namespace ui
{

CustomWidget::CustomWidget() noexcept
{
    m_images.media_type = NodeObjFlags::CustomWdgtImage;
    m_videos.media_type = NodeObjFlags::CustomWdgtVideo;
    m_audios.media_type = NodeObjFlags::CustomWdgtAudio;
}

CustomWidget::~CustomWidget() noexcept = default;

void CustomWidget::draw_custom_widget(data::NodeInstance* node, float pos, float width)
{
    if (width <= 0.0f) [[unlikely]] return;
    LogRAssert(node, "Zero ptr", );

    m_node = node;
    m_start_pos = pos;
    m_widget_width = width;

    const auto flags = m_node->obj_flags();

    if (flags.has(NodeObjFlags::CustomWdgtFromOutput))
    {
        const auto it = m_outputs.find(std::make_pair(node->get_workflow_id(), node->get_instance_id()));
        if (it != m_outputs.end())
        {
            auto& out = it->second;
            if (flags.has(NodeObjFlags::CustomWdgtImage) && NodeObjFlags::CustomWdgtImage == out.media_type)
                draw_image_widget_from_output(out);
            else if (flags.has(NodeObjFlags::CustomWdgtVideo) && NodeObjFlags::CustomWdgtVideo == out.media_type)
                draw_video_widget_from_output(out);
            else if (flags.has(NodeObjFlags::CustomWdgtAudio) && NodeObjFlags::CustomWdgtAudio == out.media_type)
                draw_audio_widget_from_output(out);
            else if (flags.has(NodeObjFlags::CustomWdgtText) && NodeObjFlags::CustomWdgtText == out.media_type)
                draw_text_widget_from_output(out);
            else
                LogError("Unhandled output widget");
        }
    }
    else
    {
        if (flags.has(NodeObjFlags::CustomWdgtUploadButton))
            draw_widget_upload_button();

        if (flags.has(NodeObjFlags::CustomWdgtFromSource) && nullptr == m_node->get_object()->custom_widget_source) [[unlikely]]
        {
            constexpr size_t err_count = 2;
            constexpr const char* err_lines[err_count] = { "Input source not set", "Please select media source in Node config" };
            draw_widget_message(err_lines, err_count);
        }
        else
        {
            if (flags.has_all(NodeObjFlags::CustomWdgtImage, NodeObjFlags::CustomWdgtFromSource))
                draw_image_widget_from_source();
            else if (flags.has_all(NodeObjFlags::CustomWdgtVideo, NodeObjFlags::CustomWdgtFromSource))
                draw_video_widget_from_source();
            else if (flags.has_all(NodeObjFlags::CustomWdgtAudio, NodeObjFlags::CustomWdgtFromSource))
                draw_audio_widget_from_source();
        }
    }

    m_node = nullptr;
}

void CustomWidget::upload_edited_image(data::NodeInstance* node, std::filesystem::path&& image_path, bool image_is_mask)
{
    m_images.upload_target = 0;
    if (node != nullptr)
        m_images.upload_target = (node->obj_flags().has(NodeObjFlags::CustomWdgtFromSource) ? node->get_instance_id() : 0);
    upload_media(m_images, image_path, image_is_mask, image_is_mask);
}

void CustomWidget::draw_widget_message(const char* const lines[], size_t count)
{
    const auto text_centered = [avail = m_widget_width, pos = m_start_pos](const char *text) noexcept
    {
        bool click = false;
        const float w = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX(pos + (avail - w) * 0.5f);
        ImGui::TextUnformatted(text);
        return click;
    };

    ImGui::Dummy(ImVec2(10.0f, 20.0f));
    for (size_t i = 0; i < count; i++)
    {
        text_centered(lines[i]);
    }
    ImGui::Dummy(ImVec2(10.0f, 20.0f));
}

void CustomWidget::draw_load_result_message(int32_t load_result)
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

void CustomWidget::check_media_list_version(Widget& widget)
{
    if (-1 == widget.list_version) [[unlikely]] // init global list
    {
        const auto& pin_list = m_node->get_object()->custom_widget_source->combo_items;
        widget.names.clear();
        std::ranges::for_each(pin_list, [&widget](const Primitive& item) { widget.names.push_back(item.get_string()); });
        widget.list_version = 0;
        return;
    }

    LogRAssert(m_node->m_media_list_version <= widget.list_version, "Local list version > global list version", );

    if (m_node->m_media_list_version == widget.list_version) [[likely]]
        return;
    m_node->m_media_list_version = widget.list_version;

    if (widget.upload_target == m_node->get_instance_id() && !widget.uploaded_media.empty())
    {
        const auto pin = m_node->get_object()->custom_widget_source;
        for (auto& input : m_node->get_inputs())
        {
            if (input->get_object()->get_hash() == pin->get_hash())
            {
                auto& pin_list = pin->combo_items;
                pin_list.push_back(widget.uploaded_media);
                pin->finalize_combo();
                input->state().value = cast::to_i64(pin_list.size() - 1);
                break;
            }
        }

        widget.uploaded_media.clear();
        widget.upload_target = 0;
    }
}

bool CustomWidget::check_source_and_get_pin(Widget& widget, int64_t& index, data::Pin*& source_pin)
{
    check_media_list_version(widget);

    if (widget.items.size() < widget.names.size())
        widget.items.resize(widget.names.size());

    source_pin = m_node->get_object()->custom_widget_source;
    if (nullptr == source_pin || source_pin->combo_items_str.empty())
        return false;

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

    if (!widget.items[index].media_valid.value_or(true) || !widget.items[index].media_can_play.value_or(true))
    {
        widget.items[index].load_result = -3;
        draw_load_result_message(widget.items[index].load_result);
        return false;
    }

    return true;
}

void CustomWidget::download_media(HttpRequestor requestor, Widget& widget, int64_t index)
{
    auto& widget_item = widget.items[index];

    if (widget_item.requested)
    {
        if (widget_item.load_result != HTTP_RESP_OK)
            draw_load_result_message(widget_item.load_result);
        return;
    }

    std::filesystem::path source_name = widget.names[index];
    const auto download_video = [&widget, w_id = m_node->get_workflow_id(), n_id = m_node->get_instance_id(), index, ext = tools::path::path_to_string(source_name.extension())]
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
    const std::string filename = tools::path::path_to_string(source_name.filename());
    const auto request = fmt::format(comfyui_http::view_input_, tools::str::url_encode(filename), tools::str::url_encode(directory));
    g::pFlow->http_get(requestor, get_alive_callback(), request.c_str(), download_video, true);
    widget_item.requested = true;
    widget_item.load_result = HTTP_RESP_IN_PROGRESS;
}

const char* CustomWidget::format_timeline(float duration, float pos)
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

void CustomWidget::reset_widgets()
{
    g::pHostManager->kill_all();
    m_images = {};
    m_videos = {};
    m_audios = {};
}

void CustomWidget::add_output(wid wf_id, iid node_id, mngr::ExecTask::OutputType type, const std::string& data, int32_t preloaded_media_id)
{
    auto wf = g::pWfsManager->access(wf_id, true);
    if (nullptr == wf)   // zero wf is not an error, it could be closed already
        return;
    auto node = wf->get_node_by_instance(node_id);
    if (nullptr == node)   // zero node is not an error, it could be removed
        return;

    node->obj_flags().add(NodeObjFlags::CustomWidget, NodeObjFlags::CustomWdgtFromOutput);
    node->reset_width();

    OutputData out;
    out.item.media_id = preloaded_media_id;
    switch (type)
    {
    case mngr::ExecTask::OutputType::Audio:
        out.media_type = NodeObjFlags::CustomWdgtAudio;
        out.item.temp_path = data;
        break;
    case mngr::ExecTask::OutputType::Gif:
        out.media_type = NodeObjFlags::CustomWdgtVideo;
        out.item.temp_path = data;
        break;
    case mngr::ExecTask::OutputType::Image:
        out.media_type = NodeObjFlags::CustomWdgtImage;
        out.item.temp_path = data;
        break;
    case mngr::ExecTask::OutputType::Text:
        out.media_type = NodeObjFlags::CustomWdgtText;
        out.item.text_info = data;
        break;
    default:
        LogRError("Unknwon output type", );
    }

    m_outputs[std::make_pair(wf_id, node_id)] = std::move(out);
}

void CustomWidget::cleanup_outputs(wid wf_id)
{
    std::erase_if(m_outputs, [wf_id]
        (const std::pair<std::pair<wid, iid>, OutputData>& value)
        {
            return value.first.first == wf_id;
        }
    );
}

void CustomWidget::cleanup_outputs(wid wf_id, iid node_id)
{
    std::erase_if(m_outputs, [wf_id, node_id]
        (const std::pair<std::pair<wid, iid>, OutputData>& value)
        {
            return value.first.first == wf_id && value.first.second == node_id;
        }
    );
}

bool CustomWidget::check_open_def_app(data::NodeInstance* node, bool check_only)
{
    const auto flags = node->obj_flags();
    if (flags.has_not(NodeObjFlags::CustomWidget))
        return false;

    if (flags.has(NodeObjFlags::CustomWdgtFromOutput))
    {
        const auto it = m_outputs.find(std::make_pair(node->get_workflow_id(), node->get_instance_id()));
        if (it != m_outputs.end())
        {
            auto& out = it->second;
            if (check_only)
                return !out.item.temp_path.empty();
            return tools::path::open_file(out.item.temp_path);
        }
        return false;
    }

    if (flags.has(NodeObjFlags::CustomWdgtFromSource) && flags.has_any(NodeObjFlags::CustomWdgtVideo, NodeObjFlags::CustomWdgtImage, NodeObjFlags::CustomWdgtAudio))
    {
        int64_t index = 0;
        data::Pin* source_pin_obj = nullptr;
        Widget* widget = nullptr;

        if (flags.has(NodeObjFlags::CustomWdgtImage))
            widget = &m_images;
        else if (flags.has(NodeObjFlags::CustomWdgtVideo))
            widget = &m_videos;
        else if (flags.has(NodeObjFlags::CustomWdgtAudio))
            widget = &m_audios;
        else
            return false;

        m_node = node;
        const bool result = check_source_and_get_pin(*widget, index, source_pin_obj);
        m_node = nullptr;
        if (!result)
            return false;

        if (check_only)
            return !widget->items[index].temp_path.empty();
        return tools::path::open_file(widget->items[index].temp_path);
    }

    return false;
}

} // namespace ui
