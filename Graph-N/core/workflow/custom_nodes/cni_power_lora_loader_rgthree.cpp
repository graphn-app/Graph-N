/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/custom_nodes/cni_power_lora_loader_rgthree.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow_action.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/history_manager.h"

namespace data
{

CNI_PowerLoraLoaderRgthree::CNI_PowerLoraLoaderRgthree(Node* obj, wid workflow_id, iid instance_id) noexcept
    : CNI_BaseObj(obj, workflow_id, instance_id)
{
    flags().add(NodeInstFlags::CNI, NodeInstFlags::JsonCustomSerltnWidgetsOnly, NodeInstFlags::BlobCustomSerltnWidgetsOnly);
}

void CNI_PowerLoraLoaderRgthree::cni_init()
{
    CNI_DeserializedPowerLoraLoaderRgthree* deserialized_values = nullptr;
    if (m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedPowerLoraLoaderRgthree>(m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values");

        if (deserialized_values != nullptr)
        {
            for (auto& lora : deserialized_values->loras)
            {
                add_lora(std::get<0>(lora), std::get<1>(lora), std::get<2>(lora));
            }
        }
    }
}

void CNI_PowerLoraLoaderRgthree::on_deserialization_ends()
{
    m_deserialized_values.reset();
}

float CNI_PowerLoraLoaderRgthree::get_additional_node_width() const noexcept
{
    const float base_w = CNI_BaseObj::get_additional_node_width();
    if (0.0f == base_w)
        return m_loras.empty() ? 0.0f : 500.0f;
    return base_w;
}

bool CNI_PowerLoraLoaderRgthree::cni_copy_from(const NodeInstance* copy_from_node, bool /*copy_pin_ids*/)
{
    if (m_loras.empty())
        return true;

    const auto* that_node = cast::strict<CNI_PowerLoraLoaderRgthree>(copy_from_node);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedPowerLoraLoaderRgthree>();
    auto loras_data = cast::strict<CNI_DeserializedPowerLoraLoaderRgthree>(data.get());

    for (const auto& lora : that_node->m_loras)
    {
        loras_data->loras.push_back(std::make_tuple(lora.name + lora.ext, lora.enabled, lora.strength));
    }

    cni_save_deserialized_info(std::move(data));
    return true;
}

void CNI_PowerLoraLoaderRgthree::add_lora(std::string lora, bool enabled, double strength /*= 1.0*/, bool add_to_history /*= false*/)
{
    blob::Blob old_val = serialize();

    Lora new_lora;
    new_lora.enabled = enabled;
    new_lora.strength = strength;
    std::filesystem::path path(lora);
    new_lora.name = path.stem().string();
    new_lora.ext = path.extension().string();
    m_loras.push_back(std::move(new_lora));

    if (add_to_history)
    {
        g::pHistoryManager->push_blob_action(get_workflow_id(), get_instance_id(), js::RGTHREE_POWER_LORA_LOADER, serialize(), std::move(old_val), "Add lora");
    }
    reset_width();
}

void CNI_PowerLoraLoaderRgthree::toggle_all()
{
    blob::Blob old_val = serialize();

    m_toggler = !m_toggler;

    for (auto& lora : m_loras)
    {
        lora.enabled = m_toggler;
    }

    g::pHistoryManager->push_blob_action(get_workflow_id(), get_instance_id(), js::RGTHREE_POWER_LORA_LOADER, serialize(), std::move(old_val), "Toggle loras");
}

void CNI_PowerLoraLoaderRgthree::enable_lora(size_t lora_index, bool enable)
{
    LogRAssert(lora_index < m_loras.size(), "Index out of bounds", );

    blob::Blob old_val = serialize();
    m_loras[lora_index].enabled = enable;
    g::pHistoryManager->push_blob_action(get_workflow_id(), get_instance_id(), js::RGTHREE_POWER_LORA_LOADER, serialize(), std::move(old_val), ("Enable " + m_loras[lora_index].name).c_str());
}

void CNI_PowerLoraLoaderRgthree::set_strength(size_t lora_index, double prev_value)
{
    LogRAssert(lora_index < m_loras.size(), "Index out of bounds", );

    const auto new_value = m_loras[lora_index].strength;
    m_loras[lora_index].strength = prev_value;
    blob::Blob old_val = serialize();
    m_loras[lora_index].strength = new_value;
    g::pHistoryManager->push_blob_action(get_workflow_id(), get_instance_id(), js::RGTHREE_POWER_LORA_LOADER, serialize(), std::move(old_val), "Lora strength");
}

blob::Blob CNI_PowerLoraLoaderRgthree::serialize() const
{
    blob::Writer writer;
    writer.push(m_toggler);
    writer.push(cast::to_u64(m_loras.size()));
    for (auto& lora : m_loras)
    {
        writer.push(lora.enabled);
        writer.push(lora.strength);
        writer.push(lora.name);
        writer.push(lora.ext);
    }
    return writer.release();
}

void CNI_PowerLoraLoaderRgthree::deserialize(const blob::Blob& blob)
{
    m_loras.clear();

    blob::Reader reader(blob);
    m_toggler = reader.pop_bool();
    const uint64_t size = reader.pop_u64();
    for (uint64_t i = 0; i < size; i++)
    {
        Lora lora;
        lora.enabled = reader.pop_bool();
        lora.strength = reader.pop_double();
        lora.name = reader.pop_string();
        lora.ext = reader.pop_string();
        m_loras.push_back(std::move(lora));
    }
}

} // namespace data


