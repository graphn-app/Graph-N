/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_js_node.h"
#include "core/workflow/custom_nodes/cni_fast_action_rgthree.h"
#include "core/workflow/custom_nodes/cni_power_lora_loader_rgthree.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/history_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow_action.h"
#include "ui/graph-n_flow.h"

namespace ui
{

void CustomJsNode::draw_FastActionsButtonRgthree(data::NodeInstance* node, float off, float width)
{
    ImGui::Spacing();
    ImGui::SetCursorPosX(off);
    if (ImGui::Button(ICON_LC_CLAPPERBOARD " Action!", ImVec2(width, 0)))
    {
        LogRAssert(node->has_object() && node->obj_flags().has(NodeObjFlags::SpecFastActRgthree), "Not an action node", );
        LogRAssert(!node->get_inputs().empty(), "Not input pins", );

        auto wf = g::pWfsManager->access(node->get_workflow_id());
        wf->handle_rgthree_switchers(node->get_inputs().front());
    }
}

void CustomJsNode::handle_insert_lora_rgthree(const data::NodeInstance* node, data::PinInstance* pin)
{
    if (pin->get_object()->def_state.name != "insert_lora" || g::pNodesManager->loras_list().empty())
        return;

    if (PinType::COMBO == pin->state().type)
    {
        const auto& lora = pin->get_object()->combo_items_str[pin->state().value.get_int()];
        if (!lora.empty() && lora != "CHOOSE" && lora != "DISABLE LORAS")
        {
            for (auto prompt : node->get_inputs())
            {
                if ("prompt" == prompt->get_object()->def_state.name)
                {
                    const auto sel = g::pFlow->txt_panel_get_selection(node->get_workflow_id(), node, prompt);
                    auto buffer = prompt->state().value.get_string_ref();

                    const auto pos = std::get<0>(sel);
                    const auto begin = std::get<1>(sel);
                    const auto end = std::get<2>(sel);

                    int32_t b = 0, n = 0;
                    if (begin != end)
                    {
                        b = std::min(begin, end);
                        n = std::max(begin, end) - b;
                    }
                    else
                        b = pos;

                    auto old = prompt->state().value;
                    buffer->replace(b, n, "<lora:" + lora + ":1.0>");

                    const auto* wf = g::pWfsManager->access(node->get_workflow_id());
                    wf->action()->multi_action_on("Insert Lora");
                    wf->action()->set_pin_val(prompt, std::move(old));
                    wf->action()->multi_action_off();

                    for (size_t i = 0; i < pin->get_object()->combo_items_str.size(); i++)
                    {
                        if ("CHOOSE" == pin->get_object()->combo_items_str[i])
                        {
                            *pin->state().value.get_int_ref() = cast::to_i64(i);
                            break;
                        }
                    }
                    break;
                }
            }            
        }
    }
}

void CustomJsNode::handle_bookmark_rgthree(const data::NodeInstance* node, data::PinInstance* /*pin*/)
{
    LogRAssert(node, "Zero ptr", );
    auto wf = g::pWfsManager->access(node->get_workflow_id());
    wf->unregister_bookmark(node);
    wf->register_bookmark(node);
}

void CustomJsNode::draw_PowerLoraLoaderRgthree(data::NodeInstance* node, float off, float width)
{
    LogRAssert(node, "Zero ptr", );
    auto lora_loader = cast::strict<data::CNI_PowerLoraLoaderRgthree>(node);
    LogRAssert(lora_loader, "Not a CNI note", );

    ImGui::Spacing();
    ImGui::SetCursorPosX(off);
    ImGui::PushID(0x01289B6B);
    ImGui::PushID(node);

    auto& loras = lora_loader->get_loras();

    if (!loras.empty())
    {
        bool toggler = lora_loader->get_toggler();
        const auto bool_type = g::pOptsManager->wf_widget_input_bools_load();
        const auto float_type = g::pOptsManager->wf_widget_input_numbers_load();

        switch (bool_type)
        {
        case WidgetInput::Toggle:
            ImGui::SetNextItemWidth(width * 0.66f - 5);
            ImGuiEx::BooleanButtons2("##toggler", "Toggle all", &toggler);
            break;
        case WidgetInput::Select:
        default:
            ImGuiEx::BooleanToggleLED("Toggle all", &toggler, false, false, width * 0.66f - 5);
            break;
        }
        if (lora_loader->get_toggler() != toggler)
            lora_loader->toggle_all();

        for (int32_t i = 0; i < loras.size(); i++)
        {
            ImGui::PushID(i + 1);
            ImGui::SetCursorPosX(off);

            auto& lora = loras[i];
            bool enabled = lora.enabled;

            switch (bool_type)
            {
            case WidgetInput::Toggle:
                ImGui::SetNextItemWidth(width * 0.66f - 5);
                ImGuiEx::BooleanButtons2("##lora", lora.name.c_str(), &enabled);
                break;
            case WidgetInput::Select:
            default:
                ImGuiEx::BooleanToggleLED(lora.name.c_str(), &enabled, false, false, width * 0.66f - 5);
                break;
            }

            ImGui::SameLine(0, 10);
            ImGui::SetNextItemWidth(width * 0.33f - 5);
            const auto backup = lora.strength;

            switch (float_type)
            {
            case WidgetInput::Direct:
            default:
                ImGuiEx::InputDoubleSmart("##strength", &lora.strength, 0.01, -100.0, 100.0, "%.02f", "%.02f");
                break;
            case WidgetInput::Drag:
                //ImGuiEx::DragDoubleSmart("##value", val, min, max, obj->precision.c_str());
            case WidgetInput::Slider:
                ImGuiEx::SliderDoubleSmart("##strength", &lora.strength, -100.0, 100.0, "%.02f");        
                break;
            }

            if (ImGui::IsItemActivated())
                lora.strength_backup = backup;
            else if (ImGui::IsItemDeactivatedAfterEdit() && lora.strength_backup != lora.strength)
                lora_loader->set_strength(i, lora.strength_backup);
            else if (lora.enabled != enabled)
                lora_loader->enable_lora(i, enabled);

            ImGui::PopID();
        }
    }

    ImGui::SetCursorPosX(off);
    if (ImGui::Button(ICON_LC_PLUS " Add Lora", ImVec2(width, 0)))
    {
        m_js_context_menu_data.id = js::RGTHREE_POWER_LORA_LOADER;
        m_js_context_menu_data.menu_id = js::RGTHREE_POWER_LORA_LOADER_CLASS;
        m_js_context_menu_data.node = node;
    }

    ImGui::PopID(2);
}

bool CustomJsNode::handle_context_menu_lora_loader_rgthree()
{
    LogRAssert(m_js_context_menu_data.node, "Zero ptr", false);

    bool close = false;
    const auto& loras = g::pNodesManager->loras_list();
    for (auto& lora : loras)
    {
        if (ImGui::Selectable(lora.c_str()))
        {
            cast::strict<data::CNI_PowerLoraLoaderRgthree>(m_js_context_menu_data.node)->add_lora(lora.c_str(), true, 1.0, true);
            close = true;
        }
    }

    return close;
}

void CustomJsNode::handle_history_lora_loader_rgthree(wid wf_id, iid node_id, const blob::Blob& value)
{
    const auto* wf = g::pWfsManager->access(wf_id);
    LogRAssert(wf, "Zero wf. w:" << wf_id, );
    const auto node = wf->get_node_by_instance(node_id);
    LogRAssert(node, "Zero node. w:" << wf_id << ", n:" << node_id, );
    cast::strict<data::CNI_PowerLoraLoaderRgthree>(node)->deserialize(value);
}

} // namespace ui
