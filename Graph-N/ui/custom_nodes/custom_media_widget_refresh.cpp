#include "ui/custom_nodes/custom_media_widget.h"
#include "core/workflow/node_instance.h"
#include "core/base/comfyui_web_paths.h"
#include "ui/graph-n_flow.h"

namespace ui
{

void CustomMediaWidget::draw_widget_refresh_button()
{
    ImGui::SetCursorPosX(m_start_pos);

    if (nullptr == m_node->get_object()->custom_widget_source)
    {
        ImGui::BeginDisabled();
        ImGui::Button("Input source not set", ImVec2(m_widget_width, 0.0f));
        ImGui::EndDisabled();
        ImGuiEx::TooltipOnLastControl("Please select media source in Node config", nullptr);
        return;
    }

    Widget* widget = nullptr;
    const auto flags = m_node->obj_flags();
    if (flags.has_all(NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaFromSourceIn))               widget = &m_images_in;
    else if (flags.has_all(NodeObjFlags::CustomMediaImage, NodeObjFlags::CustomMediaFromSourceOut))         widget = &m_images_out;
    else if (flags.has_all(NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaFromSourceIn))          widget = &m_videos_in;
    else if (flags.has_all(NodeObjFlags::CustomMediaVideo, NodeObjFlags::CustomMediaFromSourceOut))         widget = &m_videos_out;
    else if (flags.has_all(NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaFromSourceIn))          widget = &m_audios_in;
    else if (flags.has_all(NodeObjFlags::CustomMediaAudio, NodeObjFlags::CustomMediaFromSourceOut))         widget = &m_audios_out;
    else LogRError("Unhandled upload media type", );

    if (!ImGui::Button("Refresh", ImVec2(m_widget_width, 0.0f)))
        return;

    std::string request;
    if (flags.has(NodeObjFlags::CustomMediaFromSourceIn))
        request = comfyui_http::refresh_inputs;
    else
        request = comfyui_http::refresh_outputs;

    const auto callback = [this, widget](int32_t result, std::string&& data)
    {
        if (result != HTTP_RESP_OK || data.empty()) [[unlikely]] // TODO: show error
            return;

        try
        {
            widget->items.clear();
            widget->names.clear();
            widget->list_version++;
            widget->reset = true;
            const auto j = u_json::parse(data);
            if (j.is_array())
            {
                for (auto& jitem : j)
                {
                    if (jitem.is_string())
                    {
                        widget->names.push_back(jitem.get<std::string>());
                        widget->items.push_back({});
                    }
                }
            }
        }
        catch (const std::exception& ex)
        {
            LogExcep("Failed to parse downloaded list. result: " << ex.what());
        }
    };

    g::pFlow->http_get(HttpRequestor::RefreshMedia, get_alive_callback(), request.c_str(), callback);
}

} // namespace ui
