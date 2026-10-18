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
    switch (m_curr_node->get_object()->get_hash())
    {
    case js::PRIMITIVE_NODE:
        return serialize_cni_node_primitive();
    case js::REROUTE:
        return serialize_cni_node_reroute();
    case js::NOTE:
        return serialize_cni_node_note();
    case js::MARKDOWN_NOTE:
        return serialize_cni_node_markdownnote();
    case js::FAST_BYPASSER_RGTHREE:
    case js::FAST_MUTER_RGTHREE:
        return serialize_cni_node_fast_switcher_rgthree();
    case js::FAST_GROUPS_BYPASSER_RGTHREE:
    case js::FAST_GROUPS_MUTER_RGTHREE:
        return serialize_cni_node_fast_group_switcher_rgthree();
    case js::FAST_ACTIONS_BUTTON_RGTHREE:
        return serialize_cni_node_fast_action_rgthree();
    case js::BOOKMARK_RGTHREE:
        return serialize_cni_node_bookmark_rgthree();
    case js::IMPACT_SWITCH:
        return serialize_cni_node_impact_switch();
    case js::IMPACT_INVERSED_SWITCH:
        return serialize_cni_node_impact_insversed_switch();
    }

    if (m_curr_node->obj_flags().has(NodeObjFlags::CustomJsNode))
        return serialize_js_node();

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

    if (inputs.size() >= 1)
    {
        m_writer.push(inputs[0]->state().value);
        m_writer.push(cast::to_u8(inputs[0]->state().cag_val));
        m_writer.push(inputs[0]->get_object()->get_hash());
        m_writer.push(inputs[0]->get_instance_id());
    }
    else
    {
        m_writer.push(Primitive{});
        m_writer.push(cast::to_u8(ControlAfterGenerate::Fixed));
    }

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
        m_writer.push(in->get_object()->get_hash());
        m_writer.push(in->get_object()->def_state.name);
        m_writer.push(out->get_instance_id());
        m_writer.push(out->get_object()->get_hash());
        m_writer.push(out->get_object()->out_index);
        m_writer.push(in->flags().has(PinInstFlags::Hidden));
    }

    m_writer.push(m_curr_node->flags().has(NodeInstFlags::RerouteSplitted));

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

////////////////////////////////////////////////////////////////////////////////////////////
///////////    FastModeSwitcherRgthree                        //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToBlob::serialize_cni_node_fast_switcher_rgthree()
{
    blob::Block block(m_writer);

    auto sw_node = cast::strict<CNI_FastModeSwitcherRgthree>(m_curr_node);
    auto data = sw_node->dump();

    m_writer.push(data.out_id);
    m_writer.push(cast::to_u64(data.in_ids.size()));
    for (auto [id, hash, mode] : data.in_ids)
    {
        (void)hash;
        (void)mode;
        m_writer.push(id);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    FastModeGroupSwitcherRgthree                   //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToBlob::serialize_cni_node_fast_group_switcher_rgthree()
{
    blob::Block block(m_writer);

    auto sw_node = cast::strict<CNI_FastModeSwitcherRgthree>(m_curr_node);
    auto data = sw_node->dump();

    m_writer.push(data.out_id);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    FashActionRgthree                              //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToBlob::serialize_cni_node_fast_action_rgthree()
{
    blob::Block block(m_writer);

    auto sw_node = cast::strict<CNI_FastModeSwitcherRgthree>(m_curr_node);
    auto data = sw_node->dump();

    m_writer.push(cast::to_u64(data.in_ids.size()));
    for (auto [id, hash, mode] : data.in_ids)
    {
        (void)hash;
        m_writer.push(id);
        m_writer.push(mode);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    BookmarkRgthree                                //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfSerializerToBlob::serialize_cni_node_bookmark_rgthree()
{
    blob::Block block(m_writer);

    // TODO: CNI_BookmarkRgthree
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    ImpactSwitch & ImpactInversedSwitch            //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
void serialize_impact_switch(blob::Writer& writer, const CNI_DeserializedSwitchImpact& data)
{
    blob::Block block(writer);

    const auto write = [&](const std::vector<std::pair<iid, hid>>& ids)
    {
        writer.push(cast::to_u64(ids.size()));
        for (auto [id, hash] : ids)
        {
            writer.push(id);
        }
    };

    write(data.in_ids);
    write(data.out_ids);
    write(data.in_dynamic_ids);
    write(data.out_dynamic_ids);
    writer.push(data.selected);
    writer.push(data.mode);
}
} // namespace


void WfSerializerToBlob::serialize_cni_node_impact_switch()
{
    auto sw_node = cast::strict<CNI_SwitchImpact>(m_curr_node);
    serialize_impact_switch(m_writer, sw_node->dump());
}

void WfSerializerToBlob::serialize_cni_node_impact_insversed_switch()
{
    auto sw_node = cast::strict<CNI_SwitchInversedImpact>(m_curr_node);
    serialize_impact_switch(m_writer, sw_node->dump());
}

} // namespace data
