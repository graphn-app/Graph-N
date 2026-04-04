
#include "core/workflow/workflow_serializer_to_blob.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"

namespace data
{

blob::Writer WorkflowSerializerToBlob::serialize(const Workflow* wf, const blob::Blob& drawer_state)
{
    if (wf->get_data().critical_failure)
    {        
        LogInternalLogicS("Trying to serialize invalid wf " << wf->get_data().id << "###" << wf->get_data().uuid);
        return {};
    }

    blob::Writer writer;
    blob::Block root(writer);

    blob::Block block(writer);
    writer.push(wf->m_last_child_id);
    block.end();

    block.begin();
    writer.push(drawer_state);
    block.end();

    block.begin();
    writer.push(cast::to_u32(wf->m_nodes.size()));
    for (auto& node : wf->m_nodes)
    {
        serialize_node(node.get(), writer);
    }
    block.end();

    block.begin();
    writer.push(cast::to_u32(wf->m_links.size()));
    for (auto& link : wf->m_links)
    {
        serialize_link(link.get(), writer);
    }
    block.end();

    root.end();
    return writer;
}

blob::Blob WorkflowSerializerToBlob::serialize_node(const NodeInstance* node)
{
    blob::Writer writer;
    serialize_node(node, writer);
    return writer.release();
}

void WorkflowSerializerToBlob::serialize_node(const NodeInstance* node, blob::Writer& writer)
{
    blob::Block root(writer);

    blob::Block block(writer);
    writer
        .push(node->m_group ? 0 : node->m_object->get_id())
        .push(node->m_iid)
        .push(node->m_wid)
        .push(node->m_group ? "" : node->m_object->node_class)
        .push(node->get_name())
        .push(node->m_rc)
        .push(node->m_clr_node)
        .push(node->m_clr_group)
        .push(node->m_clr_head)
        .push(node->m_clr_group_capt)
        .push(node->m_group)
        .push(node->m_show_hidden)
        .push(node->m_expanded)
        .push(node->m_pinned)
        .push(cast::to_i32(node->m_mode))
    ;
    block.end();

    block.begin();
    writer.push(cast::to_i64(node->m_input.size()));
    for (auto in : node->m_input)
    {
        writer
            .push(in->m_object->get_id())
            .push(in->get_instance_id())
            .push(in->m_object->def_state.type_str)
            .push(in->m_object->def_state.name)
        ;
        serialize_pin(in, writer);
    }
    block.end();

    block.begin();
    writer.push(cast::to_i64(node->m_output.size()));
    for (auto out : node->m_output)
    {
        writer
            .push(out->m_object->get_id())
            .push(out->get_instance_id())
            .push(out->m_object->def_state.type_str)
            .push(out->m_object->def_state.name)
        ;
        serialize_pin(out, writer);
    }
    block.end();
}

void WorkflowSerializerToBlob::serialize_pin(const PinInstance* pin, blob::Writer& writer)
{
    blob::Block root(writer);
    blob::Block block(writer);

    writer
        .push(pin->get_object()->def_state.type_str)
        .push(pin->get_object()->def_state.type_hash)
        .push(cast::to_u32(pin->get_object()->def_state.type))
        .push(pin->m_obj_class)

        .push(pin->m_state.name)
        .push(pin->m_state.type_str)
        .push(pin->m_state.type_hash)
        .push(cast::to_u32(pin->m_state.type))

        .push(pin->m_state.value)
        .push(pin->m_state.cag)
    ;
}

void WorkflowSerializerToBlob::serialize_link(const Link* link, blob::Writer& writer)
{
    blob::Block block(writer);
    writer
        .push(link->m_iid)
        .push(link->m_input->get_instance_id())
        .push(link->m_output->get_instance_id())
    ;
}

bool WorkflowSerializerToBlob::deserialize(Workflow* wf, const blob::Blob& blob, blob::Blob& drawer_state)
{
    auto& data = wf->get_data();
    data.valid = true;
    data.critical_failure = false;

    try
    {
        wf->m_action->hard_reset();
        wf->clear();

        blob::Reader reader(blob);
        blob::Reader root = reader.pop_block();

        blob::Reader block = root.pop_block();
        wf->m_last_child_id = block.pop_iid();

        block = root.pop_block();
        drawer_state = block.pop_blob();

        block = root.pop_block();
        const uint32_t nodes_count = block.pop_u32();
        for (uint32_t i = 0; i < nodes_count; i++)
        {
            auto node = deserialize_node(wf, block);
            if (!node->m_group && node->m_object->is_dummy)
                data.valid = false;
        }

        block = root.pop_block();
        const uint32_t links_count = block.pop_u32();
        for (uint32_t i = 0; i < links_count; i++)
        {
            deserialize_link(wf, block);
        }
        return true;
    }
    catch (std::exception& ex)
    {
        data.critical_failure = true;
        LogExcepS(ExcepCategory::Logic, ex.what());
    }
    return false;
}

NodeInstance* WorkflowSerializerToBlob::deserialize_node(Workflow* wf, blob::Reader& reader)
{
    auto node = wf->add_blank_node();
    node->init_blank();
    deserialize_node(wf, node, reader, false);
    return node;
}

void WorkflowSerializerToBlob::deserialize_node(Workflow* wf, NodeInstance* node, blob::Reader& reader, bool dummy)
{
    auto reader_copy = reader.clone();
    blob::Reader root = reader.pop_block();

    blob::Reader block = root.pop_block();
    node->m_obj_hid = block.pop_hid();
    node->m_iid = block.pop_iid();
    node->m_wid = block.pop_wid();
    if (node->m_wid != wf->get_id())
    {
        LogAndThrowInternalErrorS(ExcepCategory::Logic, "Node wf ids don't match " << node->m_wid << " != " << wf->get_id());
    }

    const auto node_class = block.pop_string();
    const auto node_name = block.pop_string();
    node->m_rc = block.pop_rect();
    node->m_clr_node = block.pop_vec4();
    node->m_clr_group = block.pop_clr();
    node->m_clr_head = block.pop_clr();
    node->m_clr_group_capt = block.pop_clr();
    node->m_group = block.pop_bool();
    node->m_show_hidden = block.pop_bool();
    node->m_expanded = block.pop_bool();
    node->m_pinned = block.pop_bool();
    node->m_mode = cast::to_enum<NodeMode>(block.pop_i32());

    if (!node->m_group)
    {
        node->m_object = g::pNodesManager->find_node(node->m_obj_hid);
        if (node->m_object != nullptr && node->m_object->is_dummy)
        {
            LogAndThrowInternalErrorS(ExcepCategory::Logic, "Dummy node " << node_class << " in base map");
        }
        if (nullptr == node->m_object || dummy)
        {
            LogWarnS("Node " << node_class << " not found");
            node->set_object(g::pNodesManager->make_dummy_node(std::string(node_class)));
        }

        if (node->m_object->get_id() != node->m_obj_hid)
        {
            LogAndThrowInternalErrorS(ExcepCategory::Logic, "Node ids don't match " << node->m_object->get_id() << " != " << node->m_obj_hid);
        }
    }
    node->set_name(std::string(node_name));

    //if (node_name == "Negative")
    //    __debugbreak();

    const auto remove_node_pins = [wf, node]()
    {
        for (auto pin : node->m_input)
        {
            wf->remove_pin(pin);
        }
        for (auto pin : node->m_output)
        {
            wf->remove_pin(pin);
        }
        node->m_input.clear();
        node->m_output.clear();
    };

    block = root.pop_block();
    auto pins = block.pop_i64();
    for (int64_t i = 0; i < pins; i++)
    {
        const auto h_id = block.pop_hid();
        const auto i_id = block.pop_iid();
        const auto def_type = block.pop_string();
        const auto def_name = block.pop_string();
        auto pin = node->m_object->find_pin(h_id, true);
        if (node->m_object->is_dummy || nullptr == pin || pin->def_state.type_str != def_type)
        {
            if (!node->m_object->is_dummy)
            {
                remove_node_pins();
                return deserialize_node(wf, node, reader_copy, true);
            }
            pin = g::pNodesManager->make_dummy_pin(node->m_object, std::string(def_type), std::string(def_name), -1);
        }
        if (pin->get_id() != h_id)
        {
            LogAndThrowInternalErrorS(ExcepCategory::Logic, "Pin ids don't match " << pin->get_id() << " != " << h_id);
        }

        auto instance = wf->add_pin(node, pin, i_id);
        deserialize_pin(instance, block, node->m_object->is_dummy);
        node->m_input.push_back(instance);
    }

    block = root.pop_block();
    pins = block.pop_i64();
    for (int64_t i = 0; i < pins; i++)
    {
        const auto h_id = block.pop_hid();
        const auto i_id = block.pop_iid();
        const auto def_type = block.pop_string();
        const auto def_name = block.pop_string();
        auto pin = node->m_object->find_pin(h_id, false);
        if (node->m_object->is_dummy || nullptr == pin || pin->def_state.type_str != def_type)
        {
            if (!node->m_object->is_dummy)
            {
                remove_node_pins();
                return deserialize_node(wf, node, reader_copy, true);
            }
            pin = g::pNodesManager->make_dummy_pin(node->m_object, std::string(def_type), std::string(def_name), i);
        }
        if (pin->get_id() != h_id)
        {
            LogAndThrowInternalErrorS(ExcepCategory::Logic, "Pin ids don't match " << pin->get_id() << " != " << h_id);
        }

        auto instance = wf->add_pin(node, pin, i_id);
        deserialize_pin(instance, block, node->m_object->is_dummy);
        node->m_output.push_back(instance);
    }
}

void WorkflowSerializerToBlob::deserialize_pin(PinInstance* pin, blob::Reader& reader, bool dummy)
{
    blob::Reader root = reader.pop_block();
    blob::Reader block = root.pop_block();

    const auto def_type_str = block.pop_string();
    const auto def_type_hash = block.pop_u64();
    const auto def_type = cast::to_enum<PinType>(block.pop_u32());
    const auto obj_class = block.pop_string();
    if (dummy)
    {
        pin->m_object->def_state.type_str = def_type_str;
        pin->m_object->def_state.type_hash = def_type_hash;
        pin->m_object->def_state.type = def_type;
        pin->m_obj_class = obj_class;
    }
    else if (pin->m_object->def_state.type_str != def_type_str)
    {
        LogAndThrowInternalErrorS(ExcepCategory::Logic, "Pin " << def_type_str << " must be dummy");
    }

    pin->set_name(std::string(block.pop_string()));
    pin->m_state.type_str = block.pop_string();
    pin->m_state.type_hash = block.pop_u64();
    pin->m_state.type = cast::to_enum<PinType>(block.pop_u32());
    pin->m_state.value = block.pop_primitive();
    pin->m_state.cag = block.pop_iid();
}

void WorkflowSerializerToBlob::deserialize_link(Workflow* wf, blob::Reader& reader)
{
    blob::Reader block = reader.pop_block();
    const auto id = block.pop_iid();
    auto p1 = wf->get_pin_by_instance(block.pop_iid());
    auto p2 = wf->get_pin_by_instance(block.pop_iid());
    if (nullptr == p1 || nullptr == p2)
    {
        LogAndThrowInternalErrorS(ExcepCategory::Logic, "Can't create link - pin(s) not found");
    }
    if (Link::link(p1, p2, id) == nullptr)
    {
        LogAndThrowInternalErrorS(ExcepCategory::Logic, "Can't create link");
    }
}

} // namespace data
