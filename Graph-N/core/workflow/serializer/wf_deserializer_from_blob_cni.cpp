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
    if (CNI_Primitive::cni_is_same(m_curr_node))
        return deserialize_cni_node_primitive(block);
    if (CNI_Reroute::cni_is_same(m_curr_node))
        return deserialize_cni_node_reroute(block);
    if (CNI_Note::cni_is_same(m_curr_node))
        return deserialize_cni_node_note(block);
    if (CNI_MarkdownNote::cni_is_same(m_curr_node))
        return deserialize_cni_node_markdownnote(block);
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

    /*const auto inputs_count = */block.pop_u32();
    const auto outputs_count = block.pop_u32();
    if (outputs_count != 1)
        return nullptr;

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedPrimitive>();
    auto primitive_data = cast::strict<CNI_DeserializedPrimitive>(data.get());
    primitive_data->widget_value = block.pop_primitive();
    primitive_data->cag_value = block.pop_primitive();

    /*const auto h_id = */block.pop_hid();
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
    LogRAssert(m_curr_node->flags().has(NodeInstFlags::Reroute), "Node is not CNI. w:" << m_wf->get_id() << ", n: " << m_curr_node->get_instance_id() << ", t:" << typeid(m_curr_node).name(), m_curr_node);

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
        pair.in_pin_name = block.pop_string_ptr();
        pair.out_id = block.pop_iid();
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
    LogRAssertS(note->get_object()->input.size() == 1, "Invalid " << N::cni_get_node_class()  << " node definition. w:" << wf->get_id() << ", n: " << curr_node->get_instance_id() << ", t:" << typeid(curr_node).name(), curr_node);

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

} // namespace data
