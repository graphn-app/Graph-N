/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_js_node.h"
#include "core/base/comfyui_web_paths.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/custom_nodes/cni_primitive.h"
#include "core/managers/workflows_manager.h"
#include "ui/graph-n_flow.h"

namespace ui
{

void CustomJsNode::draw_CustomCombo(data::NodeInstance* /*node*/, float /*off*/, float /*width*/)
{
    // TODO
    //LogRAssert(node->m_js_data, "Data not initialized", );
    //auto data = cast::strict<js::CustomCombo_Data>(node->m_js_data.get());
    //LogRAssert(data, "Wrong data type", );
}

void CustomJsNode::draw_Load3D(data::NodeInstance* node, float off, float width)
{
    const auto find_pin = [node]() -> data::PinInstance*
    {
        for (auto pin : node->get_inputs())
        {
            if ("model_file" == pin->get_object()->def_state.name && PinType::COMBO == pin->state().type)
                return pin;
        }
        return nullptr;
    };

    const auto post_callback = [find_pin, node](int32_t result, std::string&& data)
    {
        if (result != HTTP_RESP_OK || data.empty())
            return;

        try
        {
            const auto j = u_json::parse(data);
            if (j.is_object() &&
                j.contains("name") && j["name"].is_string() &&
                j.contains("type") && j["type"].is_string() &&
                j.contains("subfolder") && j["subfolder"].is_string())
            {
                const std::string file = j["subfolder"].get<std::string>() + "/" + j["name"].get<std::string>();
                auto pin = find_pin();
                if (pin != nullptr)
                {
                    auto wf = g::pWfsManager->access(node->get_workflow_id());
                    auto action = wf->action();

                    size_t count = pin->get_object()->combo_items.size();
                    for (size_t i = 0; i < count; i++)
                    {
                        auto& val = pin->get_object()->combo_items[i];
                        if (val.get_string() == file)
                        {
                            auto old = pin->state().value;
                            *pin->state().value.get_int_ref() = cast::to_i64(i);
                            action->set_pin_val(pin, std::move(old));
                            return;
                        }
                    }

                    pin->get_object()->combo_items.push_back(file);
                    pin->get_object()->finalize_combo();

                    auto old = pin->state().value;
                    *pin->state().value.get_int_ref() = cast::to_i64(pin->get_object()->combo_items.size() - 1);   
                    action->set_pin_val(pin, std::move(old));                 
                }
            }
        }
        catch (const std::exception& ex)
        {
            LogExcepS("Failed to parse uploaded 3d. result: " << ex.what());
        }
    };

    tools::path::FdSelectResultSg file_select_callback = [this, post_callback](bool result, std::filesystem::path&& path)
    {
        if (!result)
            return;

        auto data = tools::path::read_file(path, false);
        httplib::UploadFormDataItems items =
        {
            {
                "image",
                std::move(data),
                tools::path::path_to_string(path.filename()),
                tools::img::get_mime_type(path)
            },
            { "type", "input", "", "" },
            { "subfolder", "3d", "", "" },
            { "overwrite", "true", "", "" }
        };

        g::pFlow->http_post(HttpRequestor::Upload3d, get_alive_callback(), comfyui_http::upload_media, std::move(items), post_callback);
    };

    ImGui::Spacing();
    ImGui::SetCursorPosX(off);
    if (ImGui::Button("Upload 3d model", ImVec2(width, 0)))
    {
        tools::path::select_file_open(
            {
                { L"Custom files (*.gltf, *.glb, *.obj, *.fbx, *.stl, *.ply, *.spz, *.splat, *.ksplat)", L"*.gltf;*.glb;*.obj;*.fbx;*.stl;*.ply;*.spz;*.splat;*.ksplat" },
                { L"All files (*.*)", L"*.*" },
            },
            file_select_callback);
    }
    ImGui::SetCursorPosX(off);
    if (ImGui::Button("Upload extra resources", ImVec2(width, 0)))
    {
        tools::path::select_file_open(
            {
                { L"All files (*.*)", L"*.*" },
            },
            file_select_callback);
    }
    ImGui::SetCursorPosX(off);
    if (ImGui::Button("Clear", ImVec2(width, 0)))
    {
        auto pin = find_pin();
        if (pin != nullptr)
        {
            auto wf = g::pWfsManager->access(node->get_workflow_id());
            auto action = wf->action();

            if (!pin->get_object()->combo_items.empty() && pin->get_object()->combo_items[0].get_string().empty())
            {
                auto old = pin->state().value;
                *pin->state().value.get_int_ref() = cast::to_i64(0);
                action->set_pin_val(pin, std::move(old));
                return;
            }

            pin->get_object()->combo_items.insert(pin->get_object()->combo_items.begin(), Primitive{ std::string("") });
            pin->get_object()->finalize_combo();

            auto old = pin->state().value;
            *pin->state().value.get_int_ref() = cast::to_i64(0);
            action->set_pin_val(pin, std::move(old));         
        }
    }
}

void CustomJsNode::draw_Primitive(data::NodeInstance* node, float off, float width)
{
    constexpr PinType types[] = { PinType::INT, PinType::NUMBER, PinType::FLOAT, PinType::BOOLEAN, PinType::STRING, PinType::COMBO, PinType::ANY };
    constexpr size_t count = sizeof(types) / sizeof(types[0]);
    constexpr const char* names[count] = { "INT", "NUMBER", "FLOAT", "BOOL", "STRING", "COMBO", "*" };
    const PinType curr_type = cast::strict<data::CNI_Primitive>(node)->get_current_type();

    ImGui::SetCursorPosX(off);
    for (size_t i = 0; i < count; i++)
    {
        const auto w = (i <= 2 ? width / 3 : width / 4);
        const bool new_line = (2 == i || i > count - 2);

        if (curr_type != types[i])
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_TextDisabled));
        if (ImGui::Button(names[i], ImVec2(w, 0)))
        {
            cast::strict<data::CNI_Primitive>(node)->force_set_type(types[i]);
        }
        if (curr_type != types[i])
            ImGui::PopStyleColor();
        if (!new_line)
            ImGui::SameLine(0, 0);
        else if (i < count)
            ImGui::SetCursorPosX(off);
    }
}

} // namespace ui
