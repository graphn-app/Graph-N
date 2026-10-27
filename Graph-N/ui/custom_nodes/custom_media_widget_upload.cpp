/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_media_widget.h"
#include "core/workflow/node_instance.h"
#include "core/base/comfyui_web_paths.h"
#include "ui/graph-n_flow.h"

namespace ui
{

void CustomMediaWidget::draw_widget_upload_button()
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
    if (flags.has(NodeObjFlags::CustomMediaImage))        widget = &m_images_in;
    else if (flags.has(NodeObjFlags::CustomMediaVideo))   widget = &m_videos_in;
    else if (flags.has(NodeObjFlags::CustomMediaAudio))   widget = &m_audios_in;
    else LogRError("Unhandled upload media type", );

    const auto large_file_handler = [this, widget, target = m_node->get_instance_id()](ModalDlgCtrl res)
    {
        if (ModalDlgCtrl::BtnYes == res)
        {
            widget->upload_target = target;
            upload_media(*widget, m_upload_file_path, false, true, nullptr);
        }
        m_upload_file_path.clear();
    };

    tools::path::FdSelectResultSg file_select_callback = [this, widget, large_file_handler, target = m_node->get_instance_id()](bool result, std::filesystem::path&& path)
    {
        if (!result)
            return;

        // TODO: add config
        size_t soft_limit = 0, hard_limit = 0;
        if (NodeObjFlags::CustomMediaImage == widget->media_type) { soft_limit = 20; hard_limit = 60; }
        else if (NodeObjFlags::CustomMediaVideo == widget->media_type) { soft_limit = 100; hard_limit = 500; }
        else if (NodeObjFlags::CustomMediaAudio == widget->media_type) { soft_limit = 30; hard_limit = 100; }

        constexpr uintmax_t mb = 1024 * 1024;
        const auto file_size = tools::path::get_size(path);
        const char* warning = nullptr;
        int32_t buttons = 0;
        if (file_size > hard_limit * mb)
        {
            buttons = ModalDlgCtrl::BtnYes | ModalDlgCtrl::BtnNo | ModalDlgCtrl::IcoErr;
            warning =
                "File exceeds the safety limit.\n\n"
                "Processing media of this size usually causes CUDA Out Of Memory errors or server crashes.\n\n"
                "Continue upload?"
            ;
        }
        else if (file_size > soft_limit * mb)
        {
            buttons = ModalDlgCtrl::BtnYes | ModalDlgCtrl::BtnNo | ModalDlgCtrl::IcoWarn;
            warning =
                "This file is large and may take time to upload or process\n\n"
                "Continue upload?"
            ;
        }

        if (warning != nullptr)
        {
            m_upload_file_path = std::move(path);
            g::pFlow->modal_begin("File Too Large", warning, buttons, ModalDlgCtrl::BtnNo, large_file_handler);
        }
        else
        {
            widget->upload_target = target;
            upload_media(*widget, path, false, true, nullptr);
        }
    };

    if (flags.has(NodeObjFlags::CustomMediaImage))
    {
        if (ImGui::Button("Choose image to upload...", ImVec2(m_widget_width, 0.0f)))
        {
            tools::path::select_file_open(
                {
                    { L"Image (*.png, *.jpg, *.jpeg, *.pjp, *.jfif, *.jpe, *.pjpeg, *.webp, *.gif, *.bmp)", L"*.png;*.jpg;*.jpeg;*.pjp;*.jfif;*.jpe;*.pjpeg;*.webp;*.gif;*.bmp" },
                    { L"All files (*.*)", L"*.*" },
                },
                file_select_callback);
        }
        return;
    }
    if (flags.has(NodeObjFlags::CustomMediaVideo))
    {
        if (ImGui::Button("Choose video to upload...", ImVec2(m_widget_width, 0.0f)))
        {
            tools::path::select_file_open(
                {
                    { L"Video (*.mp4, *.webm, *.mkv, *.avi, *.mov, *.gif)", L"*.mp4;*.webm;*.mkv;*.avi;*.mov;*.gif" },
                    { L"All files (*.*)", L"*.*" },
                },
                file_select_callback);
        }
        return;
    }
    if (flags.has(NodeObjFlags::CustomMediaAudio))
    {
        if (ImGui::Button("Choose audio to upload...", ImVec2(m_widget_width, 0.0f)))
        {
            tools::path::select_file_open(
                {
                    { L"Audio (*.wav, *.mp3, *.flac, *.ogg, *.opus)", L"*.wav;*.mp3;*.flac;*.ogg;*.opus" },
                    { L"All files (*.*)", L"*.*" },
                },
                file_select_callback);
        }
        return;
    }

    LogError("Unhandled upload media type");
}

void CustomMediaWidget::upload_media(Widget& widget, const std::filesystem::path& path, bool mask, bool overwrite, const std::function<void(std::string, std::string)>& callback)
{
    auto buffer = tools::path::read_file(path, false);
    if (buffer.empty())
        return;

    const auto post_callback = [this, path, &widget, callback](int32_t result, std::string&& data)
    {
        if (result != HTTP_RESP_OK || data.empty()) [[unlikely]] // TODO: show error
            return;

        try
        {
            const auto j = u_json::parse(data);
            if (j.is_object() &&
                j.contains("name") && j["name"].is_string() /*&&
                j.contains("type") && j["type"].is_string()*/)
            {
                std::string dir, name = j["name"].get<std::string>();
                widget.uploaded_media = name;
                if (j.contains("subfolder") && j["subfolder"].is_string())
                {
                    dir = j["subfolder"].get<std::string>();
                    if (!dir.empty())
                        widget.uploaded_media = dir + "/" + widget.uploaded_media;
                }

                widget.list_version++;
                widget.reset = false;

                bool found = false;
                for (size_t i = 0; i < widget.names.size(); i++)
                {
                    if (widget.names[i] == widget.uploaded_media)
                    {
                        found = true;
                        if (i < widget.items.size())
                            widget.items[i] = WidgetItem();
                        break;
                    }
                }

                if (!found)
                    widget.names.push_back(widget.uploaded_media);

                if (callback != nullptr)
                    callback(name, dir);
            }
        }
        catch (const std::exception& ex)
        {
            LogExcep("Failed to parse uploaded media. result: " << ex.what());
        }
    };

    httplib::UploadFormDataItems items =
    {
        {
            "image",
            std::move(buffer),
            tools::path::path_to_string(path.filename()),
            tools::img::get_mime_type(path)
        },
        { "type", "input", "", "" },
        { "subfolder", (mask ? "clipspace" : ""), "", "" },
        { "overwrite", (overwrite ? "true" : "false"), "", "" } 
    };

    g::pFlow->http_post(HttpRequestor::UploadMedia, get_alive_callback(), comfyui_http::upload_media, std::move(items), post_callback);
}

} // namespace ui
