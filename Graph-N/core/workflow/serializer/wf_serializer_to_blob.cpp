#include "core/workflow/serializer/wf_serializer_to_blob.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/managers/nodes_manager.h"

namespace data
{

WfSerializerToBlob::WfSerializerToBlob(const Workflow* wf) noexcept
    : m_wf(wf)
{}

blob::Writer WfSerializerToBlob::serialize(const blob::Blob& drawer_state)
{
    LogRAssert(m_wf, "Zero ptr", blob::Writer{});
    LogRAssert(!m_wf->get_data().critical_failure, "Trying to serialize invalid wf. w:" << m_wf->get_id(), blob::Writer{});

    m_writer = blob::Writer();
    blob::Block root(m_writer);

    blob::Block block(m_writer);
    m_writer.push(m_wf->get_last_child_id());
    block.end();

    block.begin();
    m_writer.push(drawer_state);
    block.end();

    block.begin();
    const auto nodes = m_wf->get_nodes_no_reroutes();
    m_writer.push(cast::to_u32(nodes.size()));
    for (auto node : nodes)
    {
        m_curr_node = node;
        serialize_node();
    }
    block.end();

    block.begin();
    const auto links = m_wf->get_links_no_reroutes();
    m_writer.push(cast::to_u32(links.size()));
    for (auto link : links)
    {
        m_curr_link = link;
        serialize_link();
    }
    block.end();

    root.end();
    return std::move(m_writer);
}

blob::Blob WfSerializerToBlob::serialize_node(const NodeInstance* node)
{
    LogRAssert(node, "Zero ptr", blob::Blob{});
    m_writer = blob::Writer();
    m_curr_node = node;
    serialize_node();
    return m_writer.release();
}

void WfSerializerToBlob::serialize_node()
{
    serialize_node_definition();

    blob::Block root(m_writer);

    blob::Block block(m_writer);
    m_writer
        .push(m_curr_node->flags().has(NodeInstFlags::Group) ? 0 : m_curr_node->get_object()->get_hash())
        .push(m_curr_node->get_instance_id())
        .push(m_curr_node->flags().has(NodeInstFlags::Group) ? "" : m_curr_node->get_object()->node_class)
        .push(m_curr_node->get_name())
        .push(m_curr_node->m_rc)
        .push(m_curr_node->m_clr_node)
        .push(m_curr_node->m_clr_group)
        .push(m_curr_node->m_clr_head)
        .push(m_curr_node->m_clr_group_capt)
        .push(m_curr_node->flags().get_by_mask(g_node_inst_seriallizable_flags))
        .push(m_curr_node->has_object() ? m_curr_node->obj_flags().get_by_mask(g_node_obj_seriallizable_flags) : uint64_t(0))
    ;
    block.end();

    if (m_curr_node->flags().has(NodeInstFlags::BlobCustomSerialization))
    {
        serialize_cni_node();
        return;
    }

    block.begin();
    m_writer.push(cast::to_u32(m_curr_node->get_inputs().size()));
    for (auto in : m_curr_node->get_inputs())
    {
        m_curr_pin = in;
        m_writer
            .push(m_curr_pin->get_object()->get_hash())
            .push(m_curr_pin->get_instance_id())
            .push(m_curr_pin->get_object()->def_state.type_str)
            .push(m_curr_pin->get_object()->def_state.name)
        ;
        serialize_pin();
    }
    block.end();

    block.begin();
    m_writer.push(cast::to_u32(m_curr_node->get_outputs().size()));
    for (auto out : m_curr_node->get_outputs())
    {
        m_curr_pin = out;
        m_writer
            .push(m_curr_pin->get_object()->get_hash())
            .push(m_curr_pin->get_instance_id())
            .push(m_curr_pin->get_object()->def_state.type_str)
            .push(m_curr_pin->get_object()->def_state.name)
        ;
        serialize_pin();
    }
    block.end();
}

void WfSerializerToBlob::serialize_pin()
{
    blob::Block root(m_writer);
    blob::Block block(m_writer);

    m_writer
        .push(m_curr_pin->get_object()->def_state.type_str)
        .push(cast::to_u32(m_curr_pin->get_object()->def_state.type))

        .push(m_curr_pin->state().name)
        .push(m_curr_pin->state().type_str)
        .push(cast::to_u32(m_curr_pin->state().type))

        .push(m_curr_pin->state().value)
        .push(m_curr_pin->state().min)
        .push(m_curr_pin->state().max)
        .push(m_curr_pin->state().step)
        .push(cast::to_u8(m_curr_pin->state().cag_val))

        .push(m_curr_pin->flags().get_by_mask(g_pin_inst_serializable_flags))
        .push(m_curr_pin->obj_flags().get_by_mask(g_pin_obj_serializable_flags))
    ;
}

void WfSerializerToBlob::serialize_link()
{
    blob::Block block(m_writer);
    m_writer
        .push(m_curr_link->get_instance_id())
        .push(m_curr_link->get_input()->get_instance_id())
        .push(m_curr_link->get_output()->get_instance_id())
    ;
}

void WfSerializerToBlob::serialize_node_definition()
{
    blob::Block block(m_writer);

    m_writer.push(m_curr_node->flags().has(NodeInstFlags::Group));
    if (m_curr_node->flags().has(NodeInstFlags::Group))
        return;

    m_writer.push(m_curr_node->flags().has(NodeInstFlags::CNI));
    m_writer.push(m_curr_node->get_object()->node_class);

    if (m_curr_node->flags().has(NodeInstFlags::BlobCustomSerialization))
    {
        serialize_cni_node_definition();
        return;
    }

    auto obj = m_curr_node->get_object();
    m_writer.push(cast::to_u32(obj->input.size()));
    m_writer.push(cast::to_u32(obj->output.size()));

    for (auto pin : obj->input)
    {
        m_writer.push(cast::to_u32(pin->def_state.type));
    }
    for (auto pin : obj->output)
    {
        m_writer.push(cast::to_u32(pin->def_state.type));
    }
}

} // namespace data
