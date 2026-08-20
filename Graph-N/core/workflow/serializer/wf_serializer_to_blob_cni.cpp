#include "core/workflow/serializer/wf_serializer_to_blob.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/pin_instance.h"

namespace data
{

void WfSerializerToBlob::serialize_cni_node_definition()
{
    // do nothing
}

void WfSerializerToBlob::serialize_cni_node()
{
    if (CNI_Primitive::cni_is_same(m_curr_node))
        return serialize_cni_node_primitive();
    if (CNI_Reroute::cni_is_same(m_curr_node))
        return serialize_cni_node_reroute();
    if (CNI_Note::cni_is_same(m_curr_node))
        return serialize_cni_node_note();
    if (CNI_MarkdownNote::cni_is_same(m_curr_node))
        return serialize_cni_node_markdownnote();
    LogError("Unhandled cni node");
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Primitive                      //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToBlob::serialize_cni_node_primitive()
{
    blob::Block block(m_writer);

    auto& inputs = m_curr_node->get_inputs();
    auto& outputs = m_curr_node->get_outputs();
    if (outputs.size() != 1)
    {
        LogWarn("Invalid node definition");
        m_writer.push(uint32_t(0));
        m_writer.push(uint32_t(0));
        return;
    }

    m_writer.push(cast::to_u32(inputs.size()));
    m_writer.push(cast::to_u32(outputs.size()));

    Primitive val;
    if (inputs.size() >= 1) val = inputs[0]->state().value;   // widget value
    m_writer.push(val);
    if (inputs.size() >= 2) val = inputs[1]->state().value;   // cag value
    else val.reset();
    m_writer.push(val);

    auto pin = outputs[0];
    m_writer.push(pin->get_object()->get_hash());
    m_writer.push(pin->get_instance_id());
    m_writer.push(pin->get_object()->def_state.type_str);
    m_writer.push(pin->state().name);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Reroute                        //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToBlob::serialize_cni_node_reroute()
{
    blob::Block block(m_writer);

    const auto& inputs = m_curr_node->get_inputs();
    const auto& outputs = m_curr_node->get_outputs();
    LogRAssert(inputs.size() == outputs.size(), "Wrong pin set. w:" << m_wf->get_id() << ", sz1:" << inputs.size() << ", sz2:" << outputs.size(), );

    m_writer.push(cast::to_u32(inputs.size()));
    for (size_t i = 0; i < inputs.size(); i++)
    {
        const auto* in = inputs[i];
        const auto* out = outputs[i];
        m_writer.push(in->state().type_str);
        m_writer.push(in->get_instance_id());
        m_writer.push(in->get_object()->def_state.name);
        m_writer.push(out->get_instance_id());
        m_writer.push(out->get_object()->out_index);
        m_writer.push(in->is_hidden());
    }

    m_writer.push(m_curr_node->is_cni_reroute_splitted());

    auto reroute = cast::strict<const data::CNI_Reroute>(m_curr_node);
    auto poses = reroute->get_splitted_nodes_poses();

    iid pin_in = 0, pin_out = 0, node = 0, link = 0;
    reroute->get_splitters(node, pin_in, pin_out, link);

    m_writer.push(node);
    m_writer.push(pin_in);
    m_writer.push(pin_out);
    m_writer.push(link);
    m_writer.push(poses.first);
    m_writer.push(poses.second);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Note, MarkdownNote             //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToBlob::serialize_cni_node_note()
{
    blob::Block block(m_writer);

    auto& inputs = m_curr_node->get_inputs();
    if (inputs.size() != 1 || inputs[0]->state().type != PinType::STRING)
    {
        LogWarn("Invalid node definition");
        m_writer.push(uint32_t(0));
        return;
    }

    m_writer.push(cast::to_u32(inputs.size()));
    m_writer.push(inputs[0]->get_instance_id());
    m_writer.push(inputs[0]->state().value);
}
void WfSerializerToBlob::serialize_cni_node_markdownnote()
{
    return serialize_cni_node_note();
}

} // namespace data
