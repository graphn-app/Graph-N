#include "core/workflow/custom_nodes/cni_note.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"

namespace data
{

std::unique_ptr<Node> CNI_Note::cni_create_object(mngr::NodesManager* manager)
{
    if (nullptr == manager)
        manager = g::pNodesManager;

    auto node = std::make_unique<data::Node>(cni_get_node_class());
    node->category = "utils";
    node->description = "Node to add annotations to a workflow.";
    node->can_mute = false;
    node->can_bypass = false;

    auto pin = manager->make_pin_ptr(node.get(), "STRING", cni_get_node_class(), -1); // input: STRING
    pin->multiline = true;
    pin->placeholder = "Put note here";
    pin->can_link = false;
    pin->can_rename = false;
    cni_add_pin(std::move(pin));

    node->finalize(nullptr);
    return node;
}

void CNI_Note::cni_cleanup_base_object() noexcept
{
    auto& ins = get_inputs();
    for (auto in : ins)
    {
        cni_remove_pin(in->get_object());
    }
}

} // namespace data
