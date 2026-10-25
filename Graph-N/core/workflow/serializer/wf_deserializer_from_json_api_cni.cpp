/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/serializer/wf_deserializer_from_json_api.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/node_instance.h"


namespace data
{

void WfDeserializerFromJsonApi::deserialize_custom_input(ApiNode& api_node)
{
    switch (api_node.node->get_object()->get_hash())
    {
    case js::RGTHREE_POWER_LORA_LOADER:
        deserialize_custom_input_rgthree_power_lora_loader(api_node);
        break;
    }
}

void WfDeserializerFromJsonApi::deserialize_custom_input_rgthree_power_lora_loader(ApiNode& api_node)
{
    LogRAssert(api_node.node, "Zero node", );
    auto lora_loader = cast::dynamic<CNI_PowerLoraLoaderRgthree>(api_node.node);
    LogRAssert(lora_loader, "Node is not CNI. n: " << api_node.node->get_instance_id() << ", t:" << typeid(api_node.node).name(), );

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedPowerLoraLoaderRgthree>();
    auto lora_data = cast::strict<CNI_DeserializedPowerLoraLoaderRgthree>(data.get());

    for (auto& [jinput_name, jinput_value] : api_node.jinputs.items())
    {
        if (jinput_name.starts_with("lora_") && jinput_value.is_object() &&
            jinput_value.contains("lora") && jinput_value["lora"].is_string() &&
            jinput_value.contains("on") && jinput_value["on"].is_boolean() &&
            jinput_value.contains("strength") && jinput_value["strength"].is_number())
        {
            const bool on = jinput_value["on"].get<bool>();
            std::string lora = jinput_value["lora"].get<std::string>();
            const float strength = jinput_value["strength"].get<float>();
            lora_data->loras.push_back(std::make_tuple(std::move(lora), on, strength));
        }
    }

    lora_loader->cni_save_deserialized_info(std::move(data));
    lora_loader->cni_init();
}

} // namespace data
