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
        node->flags.add(NodeObjFlags::CustomJsNode);
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
    }
}

void CustomJsNode::init(data::NodeInstance* node)
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

    Log("Custom JS node. " << node->get_instance_id() << ":" << node->get_object()->node_class << "/" << node->get_name());
}

void CustomJsNode::draw(data::NodeInstance* node, float off, float width)
{
    switch (node->get_object()->get_hash())
    {
    case js::VANILLA_LOAD3D:
        draw_load3d(node, off, width);
        break;
        // TODO
    //case js::CUSTOM_COMBO:
    //    draw_CustomCombo(node, off, width);
    //    break;
    case js::RGTHREE_FAST_ACTIONS_BUTTON:
        draw_FastActionsButtonRgthree(node, off, width);
        break;
    }    
}

} // namespace ui
