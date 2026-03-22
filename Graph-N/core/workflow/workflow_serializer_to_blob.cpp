
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
        .push(node->m_hid)
        .push(node->m_iid)
        .push(node->m_wid)
        .push(node->m_object->node_class)
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
        writer.push(in->m_object->get_id());
        writer.push(in->get_instance_id());
        writer.push(in->m_object->def_state.name);
    }
    block.end();

    block.begin();
    writer.push(cast::to_i64(node->m_output.size()));
    for (auto out : node->m_output)
    {
        writer.push(out->m_object->get_id());
        writer.push(out->get_instance_id());
        writer.push(out->m_object->def_state.name);
    }
    block.end();

    block.begin();
    for (auto in : node->m_input)
    {
        serialize_pin(in, writer);
    }
    block.end();

    block.begin();
    for (auto out : node->m_output)
    {
        serialize_pin(out, writer);
    }
    block.end();
}

void WorkflowSerializerToBlob::serialize_pin(const PinInstance* pin, blob::Writer& writer)
{
    blob::Block root(writer);

    blob::Block block(writer);
    writer
        .push(pin->m_hid)
        .push(pin->m_iid)
        .push(pin->m_wid)
        .push(pin->m_parent_node)
    ;
    block.end();

    block.begin();
    writer
        .push(pin->m_state.name)
        .push(pin->m_state.type_str)
        .push(pin->m_state.type_hash)
        .push(cast::to_u32(pin->m_state.type))
        .push(pin->m_state.value)
        .push(pin->m_state.cag)
    ;
    block.end();
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
        wf->m_nodes.clear();
        wf->m_pins.clear();
        wf->m_links.clear();

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
            if (node->m_object->is_dummy)
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
    deserialize_node(wf, node, reader);
    return node;
}

void WorkflowSerializerToBlob::deserialize_node(Workflow* wf, NodeInstance* node, blob::Reader& reader)
{
    blob::Reader root = reader.pop_block();

    blob::Reader block = root.pop_block();
    node->m_hid = block.pop_hid();
    node->m_iid = block.pop_iid();
    node->m_wid = block.pop_wid();
    const char* node_class = block.pop_string().data();
    node->set_name(block.pop_string().data());
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
        node->m_object = g::pNodesManager->find_node(node->m_hid);
        if (nullptr == node->m_object)
        {
            if (0 == *node_class)
                node_class = node->m_object->name.c_str();

            LogWarnS("Node " << node->m_name << " not found");
            auto obj = g::pNodesManager->make_dummy_node(node_class);
            node->m_object = obj;
        }
    }

    const auto find_pin_in = [](hid pin_id, const std::vector<Pin*>& pins) -> Pin*
    {
        for (auto pin : pins)
        {
            if (pin->get_id() == pin_id)
                return pin;
        }
        return nullptr;
    };
    const auto find_pin = [node = node->m_object, &find_pin_in](hid pin_id, bool in)
    {
        if (in)
        {
            auto pin = find_pin_in(pin_id, node->required.pins);
            if (nullptr == pin)
            {
                pin = find_pin_in(pin_id, node->optional.pins);
                if (nullptr == pin)
                {
                    pin = find_pin_in(pin_id, node->hidden.pins);
                }
            }
            return pin;
        }
        return find_pin_in(pin_id, node->output);
    };

    std::vector<std::pair<Pin*, iid>> in, out;

    block = root.pop_block();
    auto pins = block.pop_i64();
    for (int64_t i = 0; i < pins; i++)
    {
        const auto h_id = block.pop_hid();
        const auto i_id = block.pop_iid();
        const char* pin_name = block.pop_string().data();
        auto pin = find_pin(h_id, true);
        if (nullptr == pin)
        {
            LogWarnS("Serialized input Pin not found");
            pin = g::pNodesManager->make_dummy_pin(node->m_object, pin_name, -1);
            node->m_object->is_dummy = true;
        }
        in.push_back(std::make_pair(pin, i_id));
    }

    block = root.pop_block();
    pins = block.pop_i64();
    for (int64_t i = 0; i < pins; i++)
    {
        const auto h_id = block.pop_hid();
        const auto i_id = block.pop_iid();
        const char* pin_name = block.pop_string().data();
        auto pin = find_pin(h_id, false);
        if (nullptr == pin)
        {
            LogWarnS("Serialized output Pin not found");
            pin = g::pNodesManager->make_dummy_pin(node->m_object, pin_name, i);
            node->m_object->is_dummy = true;
        }
        out.push_back(std::make_pair(pin, i_id));
    }

    block = root.pop_block();
    for (auto& pin : in)
    {
        auto instance = wf->add_pin(node, pin.first, pin.second);
        deserialize_pin(instance, block);
        node->m_input.push_back(instance);
    }

    block = root.pop_block();
    for (auto& pin : out)
    {
        auto instance = wf->add_pin(node, pin.first, pin.second);
        deserialize_pin(instance, block);
        node->m_output.push_back(instance);
    }
}

void WorkflowSerializerToBlob::deserialize_pin(PinInstance* pin, blob::Reader& reader)
{
    blob::Reader root = reader.pop_block();

    blob::Reader block = root.pop_block();
    pin->m_hid = block.pop_hid();
    pin->m_object = g::pNodesManager->find_pin(pin->m_hid);
    if (nullptr == pin->m_object)
    {
        LogAndThrowInternalErrorS(ExcepCategory::Logic, "Pin " << pin->m_hid << " not found");
    }

    pin->m_state = pin->m_object->def_state;

    pin->m_iid = block.pop_iid();
    pin->m_wid = block.pop_wid();
    pin->m_parent_node = block.pop_iid();

    block = root.pop_block();
    pin->m_state.name = block.pop_string();
    pin->m_state.type_str = block.pop_string();
    pin->m_state.type_hash = block.pop_u64();
    pin->m_state.type = cast::to_enum<PinType>(block.pop_u32());
    pin->m_state.value = block.pop_primitive();
    pin->m_state.cag = block.pop_iid();

    if (pin->m_object->def_state.type_str != pin->m_state.type_str)
    {
        LogWarnS("Pin " << pin->m_state.name << " type not match. Parsed '" << pin->m_object->def_state.type_str << "', loaded '" << pin->m_state.type_str << "'");
        pin->m_object->is_dummy = true;
    }
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
