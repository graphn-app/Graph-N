#include "core/workflow/serializer/wf_serializer_to_blob.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/pin_instance.h"

namespace data
{

void WfSerializerToBlob::serialize_cni_node()
{
    if (CNI_Primitive::cni_is_same(m_curr_node))
        return serialize_cni_node_primitive();
    if (CNI_Reroute::cni_is_same(m_curr_node))
        return serialize_cni_node_reroute();
    LogError("Unhandled cni node");
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Primitive                      //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToBlob::serialize_cni_node_primitive()
{
    blob::Block block(m_writer);

    auto& intputs = m_curr_node->get_inputs();
    auto& outputs = m_curr_node->get_outputs();
    if (outputs.size() != 1)
    {
        LogWarn("Invalid node definition");
        m_writer.push(uint32_t(0));
        m_writer.push(uint32_t(0));
        return;
    }

    m_writer.push(cast::to_u32(intputs.size()));
    m_writer.push(cast::to_u32(outputs.size()));

    Primitive val;
    if (intputs.size() >= 1) val = intputs[0]->state().value;   // widget value
    m_writer.push(val);
    if (intputs.size() >= 2) val = intputs[1]->state().value;   // cag value
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
        m_writer.push(in->get_object()->hidden);
    }
}

} // namespace data
