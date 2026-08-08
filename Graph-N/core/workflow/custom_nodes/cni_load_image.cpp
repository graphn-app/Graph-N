#include "core/workflow/custom_nodes/cni_load_image.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/settings_manager.h"
#include "core/workflow/node.h"
#include "core/workflow/pin.h"
#include "core/base/comfyui_web_paths.h"
#include "ui/helpers/textures.h"
#include "ui/graph-n_flow.h"
#include "main.h"

namespace data
{

std::vector<Primitive> CNI_LoadImage::g_image_list;
int32_t CNI_LoadImage::g_image_list_version = -1;

bool CNI_LoadImage::cni_is_applicable() const noexcept
{
    auto base = get_object();
    for (auto pin : base->input)
    {
        if (pin->def_state.type == PinType::COMBO)
        {
            return pin->image_upload;
        }
    }
    LogRError("Node is not applicable", false);
}

void CNI_LoadImage::cni_draw_widget(PinInstance* pin, float pos, float width)
{
    LogRAssert(pin, "Zero ptr", );
    const auto& state = pin->state();
    auto obj = pin->get_object();

    if (state.type != PinType::COMBO || !obj->image_upload || state.links > 0)
        return;

    draw_upload_button(pos, width);
    refresh_inputs();
    check_image_list_version(pin);

    if (m_images.size() < g_image_list.size())
        m_images.resize(g_image_list.size());

    LogRAssert(obj->combo_items_ptr.size() == m_images.size(), "Images map doesn't match to combo items. w: " << get_workflow_id() << ", n:" << get_instance_id(), );

    const int64_t index = state.value.get_int();
    if (index < 0 || cast::to_szt(index) >= obj->combo_items_ptr.size())
        return;

    if (g::pTextures->get_image_alive(m_images[index].image_id))
    {
        float w = 0.0f, h = 0.0f;
        std::optional<bool> valid;
        const auto textures = g::pTextures->get_image(m_images[index].image_id, valid, &w, &h);
        if (textures && valid.has_value() && valid.value())
        {
            int32_t res_w = 0, res_h = 0;
            tools::img::scale(cast::to_i32(w), cast::to_i32(h), cast::to_i32(width), 0, res_w, res_h);
            float offset = (width / 2) - (cast::to_float(res_w) / 2) + pos;
            ImGui::SetCursorPosX(offset);
            ImGui::Image(textures, ImVec2(cast::to_float(res_w), cast::to_float(res_h)));

            if (m_images[index].or_size.empty())
            {
                int32_t or_w = 0, or_h = 0;
                g::pTextures->get_original_image_size(m_images[index].image_id, or_w, or_h);
                if (or_w > 0 && or_h > 0)
                {
                    if (or_w != or_h)
                        m_images[index].or_size = fmt::format(FMT_STRING("{} x {} [{}]"), or_w, or_h, tools::img::get_readable_aspect_ratio(or_w, or_h));
                    else
                        m_images[index].or_size = fmt::format(FMT_STRING("{} x {}"), or_w, or_h);
                    ImGui::PushFont(g::pMain->get_small_font());
                    m_images[index].or_size_w = ImGui::CalcTextSize(m_images[index].or_size.c_str()).x;
                    ImGui::PopFont();
                }
            }
            if (!m_images[index].or_size.empty())
            {
                offset = (width / 2) - (m_images[index].or_size_w / 2) + pos;
                ImGui::SetCursorPosX(offset);
                ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodePinText));
                ImGui::PushFont(g::pMain->get_small_font());
                ImGui::TextUnformatted(m_images[index].or_size.c_str(), m_images[index].or_size.c_str() + m_images[index].or_size.size());
                ImGui::PopFont();
                ImGui::PopStyleColor();
            }
        }
        return;
    }

    if (tools::path::exists(m_images[index].temp_path))
    {
        m_images[index].or_size.clear();
        m_images[index].image_id = g::pTextures->load_image(m_images[index].temp_path.c_str(), NODE_IMAGE_RECT, NODE_IMAGE_RECT, NODE_IMAGE_LIFETIME);
        return;
    }

    if (m_images[index].requested)
        return;

    std::filesystem::path source_name = obj->combo_items_ptr[index];
    const auto download_image = [this, index, ext = tools::str::path_to_string(source_name.extension())](int32_t result, std::string&& data)
    {
        if (result != HTTP_RESP_OK || data.empty())
            return;

        if (cast::to_szt(index) >= m_images.size())
        {
            LogWarn("Index out of bound. w: " << get_workflow_id() << ", n:" << get_instance_id() << ", idx:" << index << ", lst:" << m_images.size());
            return;
        }

        auto temp = fs::get_temp_directory_path();
        if (tools::path::make_folder(temp))
        {
            auto dest = tools::str::generate_uuid_v4() + ext;
            temp /= dest;
            if (tools::path::make_file(temp, true, data))
                m_images[index].temp_path = temp.string();
        }

        m_images[index].or_size.clear();
        m_images[index].image_id = g::pTextures->load_image(std::move(data), NODE_IMAGE_RECT, NODE_IMAGE_RECT, NODE_IMAGE_LIFETIME);
    };

    const std::string directory = tools::str::path_to_string(source_name.parent_path());
    const std::string filename = tools::str::path_to_string(source_name.filename());
    const auto request = fmt::format(comfyui_http::view_input_, tools::str::url_encode(filename), tools::str::url_encode(directory));
    g::pFlow->http_get("CNI_LoadImage", get_alive_callback(), request.c_str(), download_image, true);
    m_images[index].requested = true;
}

void CNI_LoadImage::draw_upload_button(float pos, float width)
{
    const auto post_callback = [this](int32_t result, std::string&& data)
    {
        if (result != HTTP_RESP_OK || data.empty())
            return;

        try
        {
            const auto j = u_json::parse(data);
            if (j.is_object() &&
                j.contains("name") && j["name"].is_string() /*&&
                j.contains("type") && j["type"].is_string()*/)
            {
                m_select_image = j["name"].get<std::string>();
                if (j.contains("subfolder") && j["subfolder"].is_string())
                {
                    const auto dir = j["subfolder"].get<std::string>();
                    if (!dir.empty())
                        m_select_image = dir + "/" + m_select_image;
                }

                bool image_exists = false;
                for (auto& img : g_image_list) // check if image already in list
                {
                    if (img.get_string() == m_select_image)
                    {
                        image_exists = true;
                        break;
                    }
                }

                if (!image_exists)
                    g_image_list.push_back(m_select_image);
                g_image_list_version++; // refresh version even if image already in list - this allows to update image selection
            }
        }
        catch (const std::exception& ex)
        {
            LogExcep("Failed to parse upload image result: " << ex.what());
        }
    };

    tools::path::FdSelectResultSg file_select_callback = [this, post_callback](bool result, std::filesystem::path&& path)
    {
        if (!result)
            return;

        auto buffer = tools::path::read_file(path, false);
        if (buffer.empty())
            return;

        httplib::UploadFormDataItems items = {
            {
                "image",
                std::move(buffer),
                tools::str::path_to_string(path.filename()),
                tools::img::get_mime_type(path)
            },
            { "type", "input", "", "" },
            { "subfolder", "", "", "" },
            { "overwrite", "false", "", "" }
        };
        
        g::pFlow->http_post("CNI_LoadImage", get_alive_callback(), comfyui_http::upload_image, std::move(items), post_callback);
    };

    ImGui::SetCursorPosX(pos);
    if (ImGui::Button("Choose file to upload", ImVec2(width, 0.0f)))
    {
        tools::path::select_file_open(
            {{ L"Image (*.png, *.jpg, *.jpeg, *.pjp, *.jfif, *.jpe, *.pjpeg, *.webp)", L"*.png;*.jpg;*.jpeg;*.pjp;*.jfif;*.jpe;*.pjpeg;*.webp" }},
            file_select_callback);
    }
}

void CNI_LoadImage::refresh_inputs() noexcept
{
 /*   if (!m_refresh_inputs)
        return;
    m_refresh_inputs = false;

    const auto callback = [this](int32_t result, std::string&& data)
    {
        if (result != HTTP_RESP_OK || data.empty())
            return;

        try
        {
            const auto j = u_json::parse(data);
            if (j.is_array())
            {
                std::vector<Primitive> vec;
                vec.reserve(j.size());
                
                for (auto& jitem : j)
                {
                    if (jitem.is_string())
                        vec.push_back(jitem.get<std::string>());
                }

                g_image_list = std::move(vec);
                g_image_list_version++;
            }
        }
        catch (const std::exception& ex)
        {
            LogExcep(ExcepCategory::ComfyUI, "Failed to parse upload image result: " << ex.what());
        }
    };

    g::pFlow->http_get("CNI_LoadImage", get_alive_callback(), comfyui_http::refresh_inputs, callback);*/
}

void CNI_LoadImage::check_image_list_version(PinInstance* pin)
{
    LogRAssert(pin, "Zero ptr", );
    if (-1 == g_image_list_version) // init global list
    {
        g_image_list = pin->get_object()->combo_items;
        g_image_list_version = 0;
        return;
    }

    LogRAssert(m_image_list_version_local <= g_image_list_version, "Local list version > global list version", );
    
    if (m_image_list_version_local == g_image_list_version)
        return;
    m_image_list_version_local = g_image_list_version;

    pin->get_object()->combo_items = g_image_list;
    pin->get_object()->finalize_combo();

    if (!m_select_image.empty())
    {
        int64_t sel = -1, idx = 0;
        for (auto& item : g_image_list)
        {
            if (!m_select_image.empty() && item.get_string().find(m_select_image) != std::string::npos)
            {
                sel = idx;
                break;
            }
            idx++;
        }

        m_select_image.clear();
        m_select_image.shrink_to_fit();

        if (sel != -1)
            pin->state().value = sel;
    }
}

} // namespace data
