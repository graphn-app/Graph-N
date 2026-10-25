/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/serializer/wf_serializer_to_json_api.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/node.h"

namespace data
{

void WfSerializerToJsonApi::serialize_custom_node(data::NodeInstance* node, o_json& jinputs)
{
    switch (node->get_object()->get_hash())
    {
    case js::RGTHREE_POWER_LORA_LOADER:
        serialize_cni_node_power_lora_loader_rgthree(node, jinputs);
        break;
    }
}

void WfSerializerToJsonApi::serialize_cni_node_power_lora_loader_rgthree(data::NodeInstance* node, o_json& jinputs)
{
    jinputs["PowerLoraLoaderHeaderWidget"]["type"] = "PowerLoraLoaderHeaderWidget";
    jinputs["➕ Add Lora"] = "";

    const auto& loras = cast::strict<CNI_PowerLoraLoaderRgthree>(node)->get_loras();
    size_t index = 1;
    for (const auto& lora : loras)
    {
        const auto name = "lora_" + std::to_string(index++);
        jinputs[name]["on"] = lora.enabled;
        jinputs[name]["lora"] = lora.name + lora.ext;
        jinputs[name]["strength"] = lora.strength;
    }
}

} // namespace data
