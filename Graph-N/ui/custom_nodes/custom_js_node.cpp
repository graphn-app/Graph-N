/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/custom_nodes/custom_js_node.h"
#include "core/managers/nodes_manager.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/node.h"
#include "core/workflow/pin.h"

namespace ui
{

void CustomJsNode::test(mngr::NodesManager* manager, data::Node* node)
{
    switch (node->get_hash())
    {
    case js::VANILLA_LOAD3D:
    //case js::VANILLA_PRIMITIVE_NODE:
        node->flags.add(NodeObjFlags::CustomJsNode);
        break;
    case js::VANILLA_LORA_LOADER:
        for (auto pin : node->input)
        {
            if (PinType::COMBO == pin->def_state.type && "lora_name" == pin->def_state.name)
            {
                auto& list = manager->loras_list();
                list.reserve(pin->combo_items_str.size());
                for (auto& lora : pin->combo_items_str)
                {
                    list.push_back(lora);
                }
                break;
            }
        }
        break;
        // TODO
    //case js::CUSTOM_COMBO:
    //    if (!node->input.empty())
    //        node->input[0]->flags.remove(PinObjFlags::CanLink, PinObjFlags::CanUnlink, PinObjFlags::CanRename);
    //    node->flags.add(NodeObjFlags::CustomJsNode);
    //    break;

    case js::RGTHREE_CONTEXT_MERGE:
    case js::RGTHREE_CONTEXT_MERGE_BIG:
    case js::RGTHREE_CONTEXT_SWITCH:
    case js::RGTHREE_CONTEXT_SWITCH_BIG:
        for (size_t i = 1; i <= 5; i++)
        {
            manager->make_pin(node, "RGTHREE_CONTEXT", fmt::format(FMT_STRING("ctx_0{}"), i), -1);
        }
        break;
    case js::RGTHREE_IMAGE_OR_LATENT_SIZE:
        manager->make_pin(node, "*", "input", -1);
        break;
    case js::RGTHREE_SDXL_POWER_PROMPT_SIMPLE_NEG:
    case js::RGTHREE_SDXL_POWER_PROMPT_POS:
    case js::RGTHREE_POWER_PROMPT:
    case js::RGTHREE_POWER_PROMPT_SIMPLE:
        {
            node->flags.add(NodeObjFlags::CustomJsNode);
            auto pins = node->input;
            for (auto pin : pins)
            {
                if ("insert_embedding"      == pin->def_state.name ||
                    "insert_saved"          == pin->def_state.name ||
                    "target_width"          == pin->def_state.name ||
                    "target_height"         == pin->def_state.name ||
                    "crop_width"            == pin->def_state.name ||
                    "crop_height"           == pin->def_state.name ||
                    "values_insert_saved"   == pin->def_state.name)
                {
                    manager->remove_pin(node, pin);
                }
                else if ("insert_lora"      == pin->def_state.name)
                {
                    pin->flags.add(PinObjFlags::NoHistoryOnValChange);
                }
            }
        }
        break;
    case js::RGTHREE_POWER_LORA_LOADER:
        node->flags.add(NodeObjFlags::CustomJsNode);
        break;
    }
}

void CustomJsNode::init(data::NodeInstance* /*node*/)
{
    //switch (node->get_object()->get_hash())
    //{
    //    // TODO
    ////case js::CUSTOM_COMBO:
    ////    node->flags().add(NodeInstFlags::JsonCustomSerialization, NodeInstFlags::BlobCustomSerialization);
    ////    node->m_js_data = std::make_unique<js::CustomCombo_Data>();
    ////    break;
    //default:
    //    return;
    //}

    //Log("Custom JS node. " << node->get_instance_id() << ":" << node->get_object()->node_class << "/" << node->get_name());
}

void CustomJsNode::draw(data::NodeInstance* node, float off, float width)
{
    switch (node->get_object()->get_hash())
    {
    case js::VANILLA_LOAD3D:
        draw_Load3D(node, off, width);
        break;
    case js::VANILLA_PRIMITIVE_NODE:
        draw_Primitive(node, off, width);
        break;
        // TODO
    //case js::CUSTOM_COMBO:
    //    draw_CustomCombo(node, off, width);
    //    break;
    case js::RGTHREE_FAST_ACTIONS_BUTTON:
        draw_FastActionsButtonRgthree(node, off, width);
        break;
    case js::RGTHREE_POWER_LORA_LOADER:
        draw_PowerLoraLoaderRgthree(node, off, width);
        break;
    }
}

void CustomJsNode::handle_pin_value_changed(data::NodeInstance* node, data::PinInstance* pin)
{
    switch (node->get_object()->get_hash())
    {
    case js::RGTHREE_SDXL_POWER_PROMPT_POS:
    case js::RGTHREE_POWER_PROMPT:
        handle_insert_lora_rgthree(node, pin);
        break;
    }
}

bool CustomJsNode::handle_context_menu()
{
    if (0 == m_js_context_menu_data.id) [[likely]]
        return false;
    LogRAssert(m_js_context_menu_data.menu_id, "Zero menu id", false);

    if (!m_js_context_menu_data.menu_inited)
    {
        m_js_context_menu_data.menu_inited = true;
        ImGui::OpenPopup(m_js_context_menu_data.menu_id);
    }

    bool close_menu = false;
    const bool menu_open = ImGui::BeginPopup(m_js_context_menu_data.menu_id);
    if (menu_open)
    {
        switch (m_js_context_menu_data.id)
        {
        case js::RGTHREE_POWER_LORA_LOADER:
            close_menu = handle_context_menu_lora_loader_rgthree();
            break;
        default:
            close_menu = true;
            LogError("Unhandled js menu");
        }

        if (close_menu)
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    if (!menu_open || close_menu)
        m_js_context_menu_data = {};

    return true;
}

void CustomJsNode::handle_history(uint64_t id, wid wf_id, iid node_id, const blob::Blob& value)
{
    switch (id)
    {
    case js::RGTHREE_POWER_LORA_LOADER:
        handle_history_lora_loader_rgthree(wf_id, node_id, value);
        break;
    }
}

} // namespace ui
