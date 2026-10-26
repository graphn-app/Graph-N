/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/serializer/wf_deserializer_from_blob.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"

namespace data
{

bool WfDeserializerFromBlob::check_cni_node_definition(blob::Reader& /*block*/, const std::string& /*node_class*/) const
{
    // do nothing
    return true;
}

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node(blob::Reader& block)
{
    switch (m_curr_node->get_object()->get_hash())
    {
    case js::VANILLA_PRIMITIVE_NODE:
        return deserialize_cni_node_primitive(block);
    case js::VANILLA_REROUTE:
        return deserialize_cni_node_reroute(block);
    case js::VANILLA_NOTE:
        return deserialize_cni_node_note(block);
    case js::VANILLA_MARKDOWN_NOTE:
        return deserialize_cni_node_markdownnote(block);
    case js::RGTHREE_FAST_BYPASSER:
    case js::RGTHREE_FAST_MUTER:
        return deserialize_cni_node_fast_switcher_rgthree(block);
    case js::RGTHREE_FAST_GROUPS_BYPASSER:
    case js::RGTHREE_FAST_GROUPS_MUTER:
        return deserialize_cni_node_fast_group_switcher_rgthree(block);
    case js::RGTHREE_FAST_ACTIONS_BUTTON:
        return deserialize_cni_node_fast_action_rgthree(block);
    case js::RGTHREE_BOOKMARK:
        return deserialize_cni_node_bookmark_rgthree(block);
    case js::RGTHREE_POWER_LORA_LOADER:
        return deserialize_cni_node_power_lora_loader_rgthree(block);
    case js::IMPACT_SWITCH:
        return deserialize_cni_node_impact_switch(block);
    case js::IMPACT_INVERSED_SWITCH:
        return deserialize_cni_node_impact_insversed_switch(block);
    }

    if (m_curr_node->obj_flags().has(NodeObjFlags::CustomJsNode))
        return deserialize_js_node(block);

    LogRError("Unhandled cni node. n:" << m_curr_node->get_object()->node_class, nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Primitive                      //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_primitive(blob::Reader& root_block)
{
    auto primitive = cast::dynamic<CNI_Primitive>(m_curr_node);
    LogRAssert(primitive, "Node is not CNI. w:" << m_wf->get_id() << ", n: " << m_curr_node->get_instance_id() << ", t:" << typeid(m_curr_node).name(), m_curr_node);

    auto block = root_block.pop_block();

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedPrimitive>();
    auto primitive_data = cast::strict<CNI_DeserializedPrimitive>(data.get());

    primitive_data->widget_value = block.pop_primitive();
    primitive_data->cag_value = cast::to_enum<ControlAfterGenerate>(block.pop_u8());
    primitive_data->widget_hash = block.pop_hid();
    primitive_data->widget_id = block.pop_iid();

    primitive_data->out_hash = block.pop_hid();
    primitive_data->out_id = block.pop_iid();
    primitive_data->out_pin_type = block.pop_string_ptr();
    primitive_data->out_pin_name = block.pop_string_ptr();

    primitive->cni_save_deserialized_info(std::move(data));
    primitive->cni_init();
    return m_curr_node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Reroute                        //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_reroute(blob::Reader& root_block)
{
    LogRAssert(m_curr_node->obj_flags().has(NodeObjFlags::SpecReroute), "Node is not CNI. w:" << m_wf->get_id() << ", n: " << m_curr_node->get_instance_id() << ", t:" << typeid(m_curr_node).name(), m_curr_node);

    auto block = root_block.pop_block();
    const auto count = block.pop_u32();
    LogRAssert(count > 0, "Null pins saved", m_curr_node);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedReroute>();
    auto reroute_data = cast::strict<CNI_DeserializedReroute>(data.get());
    reroute_data->pairs.reserve(count);
    for (uint32_t i = 0; i < count; i++)
    {
        RoutedPinPair pair;
        pair.type = block.pop_string_ptr();
        pair.in_id = block.pop_iid();
        pair.in_hash = block.pop_hid();
        pair.in_pin_name = block.pop_string_ptr();
        pair.out_id = block.pop_iid();
        pair.out_hash = block.pop_hid();
        pair.out_index = block.pop_i32();
        pair.hidden = block.pop_bool();
        reroute_data->pairs.push_back(pair);
    }

    reroute_data->splitted = block.pop_bool();
    reroute_data->splitted_node = block.pop_iid();
    reroute_data->splitter_in = block.pop_iid();
    reroute_data->splitter_out = block.pop_iid();
    reroute_data->splitter_link = block.pop_iid();
    reroute_data->poses = { block.pop_rect(), block.pop_rect() };

    m_curr_node->cni_save_deserialized_info(std::move(data));
    m_curr_node->cni_init();

    return m_curr_node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Note, MarkdownNote             //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

template<typename N>
NodeInstance* deserialize_cni_node_note_impl(blob::Reader& root_block, Workflow* wf, NodeInstance* curr_node)
{
    auto note = cast::dynamic<N>(curr_node);
    LogRAssertS(note, "Node is not CNI. w:" << wf->get_id() << ", n: " << curr_node->get_instance_id() << ", t:" << typeid(curr_node).name(), curr_node);
    LogRAssertS(note->get_object()->input.size() == 1, "Invalid " << note->get_object()->node_class  << " node definition. w:" << wf->get_id() << ", n: " << curr_node->get_instance_id() << ", t:" << typeid(curr_node).name(), curr_node);

    auto block = root_block.pop_block();

    const auto inputs_count = block.pop_u32();
    if (inputs_count != 1)
        return nullptr;

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedNote>();
    auto note_data = cast::strict<CNI_DeserializedNote>(data.get());
    note_data->id = block.pop_iid();
    note_data->widget_value = block.pop_primitive();

    note->cni_save_deserialized_info(std::move(data));
    note->cni_init();
    return curr_node;
}

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_note(blob::Reader& root_block)
{
    return deserialize_cni_node_note_impl<CNI_Note>(root_block, m_wf, m_curr_node);
}

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_markdownnote(blob::Reader& root_block)
{
    return deserialize_cni_node_note_impl<CNI_MarkdownNote>(root_block, m_wf, m_curr_node);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    CNI_FastModeSwitcherRgthree                     /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_fast_switcher_rgthree(blob::Reader& root_block)
{
    LogRAssert(m_curr_node->obj_flags().has_any(NodeObjFlags::SpecFastBypasserRgthree, NodeObjFlags::SpecFastMuterRgthree), "Node is not CNI. w:" << m_wf->get_id() << ", n: " << m_curr_node->get_instance_id() << ", t:" << typeid(m_curr_node).name(), m_curr_node);

    auto block = root_block.pop_block();

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedFastActionRgthree>();
    auto fast_data = cast::strict<CNI_DeserializedFastActionRgthree>(data.get());

    fast_data->out_id = block.pop_iid();

    const auto size = block.pop_u64();
    for (auto i = 0; i < size; i++)
    {
        fast_data->in_ids.push_back(std::make_tuple(block.pop_iid(), 0, 0));
    }

    m_curr_node->cni_save_deserialized_info(std::move(data));
    m_curr_node->cni_init();

    return m_curr_node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    CNI_FastModeGroupSwitcherRgthree                     /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_fast_group_switcher_rgthree(blob::Reader& root_block)
{
    LogRAssert(m_curr_node->obj_flags().has_any(NodeObjFlags::SpecFastGBypasserRgthree, NodeObjFlags::SpecFastGMuterRgthree), "Node is not CNI. w:" << m_wf->get_id() << ", n: " << m_curr_node->get_instance_id() << ", t:" << typeid(m_curr_node).name(), m_curr_node);

    auto block = root_block.pop_block();

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedFastActionRgthree>();
    auto fast_data = cast::strict<CNI_DeserializedFastActionRgthree>(data.get());
    fast_data->out_id = block.pop_iid();
    m_curr_node->cni_save_deserialized_info(std::move(data));
    m_curr_node->cni_init();

    return m_curr_node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    CNI_FastActionRgthree                           /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_fast_action_rgthree(blob::Reader& root_block)
{
    LogRAssert(m_curr_node->obj_flags().has(NodeObjFlags::SpecFastActRgthree), "Node is not CNI. w:" << m_wf->get_id() << ", n: " << m_curr_node->get_instance_id() << ", t:" << typeid(m_curr_node).name(), m_curr_node);

    auto block = root_block.pop_block();

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedFastActionRgthree>();
    auto fast_data = cast::strict<CNI_DeserializedFastActionRgthree>(data.get());

    const auto size = block.pop_u64();
    for (auto i = 0; i < size; i++)
    {
        const auto id = block.pop_iid();
        const auto mode = block.pop_u8();
        fast_data->in_ids.push_back(std::make_tuple(id, 0, mode));
    }

    m_curr_node->cni_save_deserialized_info(std::move(data));
    m_curr_node->cni_init();

    return m_curr_node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    CNI_BookmarkRgthree                             /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_bookmark_rgthree(blob::Reader& /*root_block*/)
{
    // TODO: CNI_BookmarkRgthree
    //LogRAssert(m_curr_node->obj_flags().has(NodeObjFlags::SpecBookmarkRgthree), "Node is not CNI. w:" << m_wf->get_id() << ", n: " << m_curr_node->get_instance_id() << ", t:" << typeid(m_curr_node).name(), m_curr_node);

    return m_curr_node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    CNI_PowerLoraLoaderRgthree                      /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_power_lora_loader_rgthree(blob::Reader& root_block)
{
    auto block = root_block.pop_block();

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedPowerLoraLoaderRgthree>();
    auto loras_data = cast::strict<CNI_DeserializedPowerLoraLoaderRgthree>(data.get());

    const auto size = block.pop_u64();
    for (auto i = 0; i < size; i++)
    {
        const auto lora = block.pop_string();
        const auto enabled = block.pop_bool();
        const auto strength = block.pop_double();
        loras_data->loras.push_back(std::make_tuple(lora, enabled, strength));
    }

    m_curr_node->cni_save_deserialized_info(std::move(data));
    m_curr_node->cni_init();

    return m_curr_node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    ImpactSwitch & ImpactInversedSwitch            //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
void deserialize_impact_switch(blob::Reader& root_block, CNI_DeserializedSwitchImpact& data)
{
    auto block = root_block.pop_block();

    const auto read = [&](std::vector<std::pair<iid, hid>>& ids)
    {
        const auto size = block.pop_u64();
        for (auto i = 0; i < size; i++)
        {
            const auto id = block.pop_iid();
            ids.push_back(std::make_pair(id, 0));
        }
    };

    read(data.in_ids);
    read(data.out_ids);
    read(data.in_dynamic_ids);
    read(data.out_dynamic_ids);
    data.selected = block.pop_i64();
    data.mode = block.pop_bool();
}
} // namespace


NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_impact_switch(blob::Reader& root_block)
{
    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedSwitchImpact>();
    auto sw_data = cast::strict<CNI_DeserializedSwitchImpact>(data.get());
    deserialize_impact_switch(root_block, *sw_data);
    m_curr_node->cni_save_deserialized_info(std::move(data));
    m_curr_node->cni_init();
    return m_curr_node;
}

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_impact_insversed_switch(blob::Reader& root_block)
{
    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedSwitchImpact>();
    auto sw_data = cast::strict<CNI_DeserializedSwitchImpact>(data.get());
    deserialize_impact_switch(root_block, *sw_data);
    m_curr_node->cni_save_deserialized_info(std::move(data));
    m_curr_node->cni_init();
    return m_curr_node;
}

} // namespace data
