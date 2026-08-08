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
    ((CNI_DeserializedPrimitive*)data.get())->widget_value = block.pop_primitive();
    ((CNI_DeserializedPrimitive*)data.get())->cag_value = block.pop_primitive();

    /*const auto h_id = */block.pop_hid();
    ((CNI_DeserializedPrimitive*)data.get())->out_id = block.pop_iid();
    ((CNI_DeserializedPrimitive*)data.get())->out_pin_type = block.pop_string();
    ((CNI_DeserializedPrimitive*)data.get())->out_pin_name = block.pop_string();

    primitive->cni_save_deserialized_info(std::move(data));
    primitive->cni_init();
    return m_curr_node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Reroute                        //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

NodeInstance* WfDeserializerFromBlob::deserialize_cni_node_reroute(blob::Reader& root_block)
{
    auto reroute = cast::dynamic<CNI_Reroute>(m_curr_node);
    LogRAssert(reroute, "Node is not CNI. w:" << m_wf->get_id() << ", n: " << m_curr_node->get_instance_id() << ", t:" << typeid(m_curr_node).name(), m_curr_node);

    auto block = root_block.pop_block();
    const auto count = block.pop_u32();
    LogRAssert(count > 0, "Null pins saved", m_curr_node);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedReroute>();
    ((CNI_DeserializedReroute*)data.get())->pairs.reserve(count);
    for (uint32_t i = 0; i < count; i++)
    {
        RoutedPinPair pair;
        pair.type = block.pop_string();
        pair.in_id = block.pop_iid();
        pair.in_pin_name = block.pop_string();
        pair.out_id = block.pop_iid();
        pair.out_index = block.pop_i32();
        pair.hidden = block.pop_bool();
        ((CNI_DeserializedReroute*)data.get())->pairs.push_back(pair);
    }

    m_curr_node->cni_save_deserialized_info(std::move(data));
    reroute->cni_init();
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
    ((CNI_DeserializedNote*)data.get())->id = block.pop_iid();
    ((CNI_DeserializedNote*)data.get())->widget_value = block.pop_primitive();

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
