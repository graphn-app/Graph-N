#include "ui/custom_nodes/custom_js_node.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/node.h"

namespace ui
{

void CustomJsNode::test(data::Node* /*node*/)
{
    //switch (node->get_hash())
    //{
    //case js::FastActionsButtonRgthree.hash:
    //    node->flags.add(NodeObjFlags::CustomJsNode);
    //    break;
    //}
}

void CustomJsNode::init(data::NodeInstance* node)
{
    //switch (node->get_object()->get_hash())
    //{
    ////case js::FastActionsButtonRgthree.hash:
    //    // do nothing
    //    //break;
    //default:
    //    return;
    //}

    Log("Custom JS node. " << node->get_instance_id() << ":" << node->get_object()->node_class << "/" << node->get_name());

    //node->flags().add(NodeInstFlags::JsonCustomSerialization, NodeInstFlags::BlobCustomSerialization);
    //if (js::FastActionsButtonRgthree.need_data)
    //    node->m_js_data = std::make_unique<js::Js_LoadImageFromUrl>();
}

void CustomJsNode::draw(data::NodeInstance* node, float off, float width)
{
    switch (node->get_object()->get_hash())
    {
    case js::FastActionsButtonRgthree.hash:
        draw_FastActionsButtonRgthree(node, off, width);
        break;
    }

    //draw_LoadImageFromUrl(node, cast::strict<js::Js_LoadImageFromUrl>(node->m_js_data.get()));
}

} // namespace ui
