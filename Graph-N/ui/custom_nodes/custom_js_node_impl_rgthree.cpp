/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_js_node.h"
#include "core/workflow/custom_nodes/cni_fast_action_rgthree.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
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

void CustomJsNode::handle_insert_lora_rgthree(data::NodeInstance* node, data::PinInstance* pin)
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

                    auto wf = g::pWfsManager->access(node->get_workflow_id());
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

} // namespace ui
