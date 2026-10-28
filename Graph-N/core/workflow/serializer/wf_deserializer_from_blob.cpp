/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/serializer/wf_deserializer_from_blob.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"

namespace data
{

WfDeserializerFromBlob::WfDeserializerFromBlob(Workflow* wf) noexcept
    : m_wf(wf)
    , m_reader({})
{
}

WfDeserializerFromBlob::WfDeserializerFromBlob(Workflow* wf, const blob::Blob& blob) noexcept
    : m_wf(wf)
    , m_reader(blob)
{
}

bool WfDeserializerFromBlob::deserialize(blob::Blob& drawer_state)
{
    LogRAssert(m_wf, "Zero ptr", false);
    LogRAssert(!m_reader.empty(), "Zero blob", false);

    auto& data = m_wf->get_data();
    data.valid = true;
    data.critical_failure = false;

    try
    {
        m_wf->action()->hard_reset();
        m_wf->clear_wf(); 

        blob::Reader root = m_reader.pop_block();

        blob::Reader block = root.pop_block();
        m_wf->set_last_child_id(block.pop_iid());

        block = root.pop_block();
        drawer_state = block.pop_blob();

        block = root.pop_block();
        const uint32_t nodes_count = block.pop_u32();

        auto nodes_block = block.clone();
        for (uint32_t i = 0; i < nodes_count; i++)
        {
            const bool dummy = !check_node_definition(nodes_block);
            if (dummy) data.valid = false;
            const auto* node = deserialize_node(nodes_block, dummy);
            LogAssert(node != nullptr, "Checked node is dummy");
        }

        block = root.pop_block();
        const uint32_t links_count = block.pop_u32();
        for (uint32_t i = 0; i < links_count; i++)
        {
            deserialize_link(block);
        }

        const auto nodes = m_wf->get_nodes_no_reroutes_r();
        for (auto node : nodes)
        {
            node->flags().remove(NodeInstFlags::Deserialization);
            node->on_deserialization_ends();
        }

#ifdef _DEBUG
        int32_t dup_errs = 0;
        std::set<iid> ids;

        for (auto& node : nodes)
        {
            if (node->flags().has(NodeInstFlags::Group))
                continue;

            if (!ids.insert(node->get_instance_id()).second)
            {
                if (0 == dup_errs)
                    LogError("Duplicate node id. w:" << m_wf->get_id() << ", n:" << node->get_instance_id());
                dup_errs++;
            }
            if (node->flags().has(NodeInstFlags::RerouteSplitted_R))
                continue;
            auto& input = node->get_inputs();
            for (auto pin : input)
            {
                if (!ids.insert(pin->get_instance_id()).second)
                {
                    if (0 == dup_errs)
                        LogError("Duplicate pin id. w:" << m_wf->get_id() << ", n:" << node->get_instance_id() << ", pi:" << pin->get_instance_id());
                    dup_errs++;
                }
            }
            auto& output = node->get_outputs();
            for (auto pin : output)
            {
                if (!ids.insert(pin->get_instance_id()).second)
                {
                    if (0 == dup_errs)
                        LogError("Duplicate pin id. w:" << m_wf->get_id() << ", n:" << node->get_instance_id() << ", po:" << pin->get_instance_id());
                    dup_errs++;
                }
            }
        }
#endif
        return true;
    }
    catch (const std::exception& ex)
    {
        data.critical_failure = true;
        LogExcep("Blob deserialization ex: " << ex.what());
    }
    return false;
}

NodeInstance* WfDeserializerFromBlob::deserialize_node(blob::Reader& block)
{
    LogRAssert(m_wf, "Zero ptr", nullptr);
    const bool dummy = !check_node_definition(block);
    auto node = deserialize_node(block, dummy);
    if (node != nullptr)
    {
        node->flags().remove(NodeInstFlags::Deserialization);
        node->on_deserialization_ends();
    }
    return node;
}

NodeInstance* WfDeserializerFromBlob::deserialize_node(blob::Reader& root_block, bool dummy)
{
    blob::Reader root = root_block.pop_block();
    blob::Reader block = root.pop_block();

    const auto obj_hid = block.pop_hid();
    const auto obj_iid = block.pop_iid();
    const auto node_class = block.pop_string();
    auto node_name = block.pop_string();

    //BREAK_IF(node_class == "KSampler");

    m_curr_node = nullptr;
    if (0 == obj_hid)
    {
        m_curr_node = m_wf->add_group_node();
    }
    else if (obj_hid != 0)
    {
        if (dummy)
        {
            m_curr_node = m_wf->add_node(g::pNodesManager->make_dummy_node(node_class), {}, {}, obj_iid);
            m_curr_node->reset_pins();
        }
        else
        {
            auto obj = g::pNodesManager->find_node(obj_hid);
            LogRAssert(obj != nullptr, "Dummy node " << node_class << " in base map", nullptr);
            m_curr_node = m_wf->add_node(obj, {}, {}, obj_iid);
            m_curr_node->flags().add(NodeInstFlags::Deserialization);
        }
    }

    m_curr_node->set_name(std::move(node_name));
    m_curr_node->m_rc = block.pop_rect();
    m_curr_node->m_clr_node = block.pop_clr();
    m_curr_node->m_clr_group = block.pop_clr();
    m_curr_node->m_clr_head = block.pop_clr();
    m_curr_node->m_clr_group_capt = block.pop_clr();
    m_curr_node->flags().add_raw(block.pop_u64());
    const auto obj_flags = block.pop_u64();
    if (m_curr_node->has_object())
        m_curr_node->obj_flags().add_raw(obj_flags);

    m_curr_node->cni_cleanup_base_object();
    for (auto pin : m_curr_node->get_inputs())
    {
        m_wf->remove_pin(pin);
    }
    for (auto pin : m_curr_node->get_outputs())
    {
        m_wf->remove_pin(pin);
    }
    m_curr_node->reset_pins();

    //BREAK_IF(m_curr_node->get_name() == "CLIP Conditioning");

    if (!dummy && m_curr_node->flags().has(NodeInstFlags::BlobCustomSerialization))
    {
        if (nullptr == deserialize_cni_node(root))
        {
            m_wf->remove_node(m_curr_node);
            LogRError("Dummy CNI node " << node_class, nullptr);
        }
        return m_curr_node;
    }

    const auto unpack_pins = [this, dummy](blob::Reader block, bool input_pin)
    {
        const auto pin_count = cast::to_i32(block.pop_u32());
        for (int32_t idx = 0; idx < pin_count; idx++)
        {
            const auto h_id = block.pop_hid();
            const auto i_id = block.pop_iid();
            const auto def_type = block.pop_string();
            const auto def_name = block.pop_string();

            Pin* pin = nullptr;
            if (dummy)
            {
                pin = g::pNodesManager->make_dummy_pin(m_curr_node->get_object(), def_type, def_name, input_pin ? -1 : idx);
            }
            else
            {
                pin = g::pNodesManager->find_pin(h_id);
                if (nullptr == pin)
                    pin = g::pNodesManager->make_pin(m_curr_node->get_object(), def_type, def_name, input_pin ? -1 : idx);
            }

            m_curr_pin = m_wf->add_pin(m_curr_node, pin, i_id);
            if (!deserialize_pin(block, dummy))
                return false;

            if (input_pin)
                m_curr_node->add_input(m_curr_pin);
            else
                m_curr_node->add_output(m_curr_pin);
        }
        return true;
    };

    if (!unpack_pins(root.pop_block(), true) || !unpack_pins(root.pop_block(), false))
    {
        m_wf->remove_node(m_curr_node);
        return nullptr;
    }

    if (!dummy && m_curr_node->flags().has(NodeInstFlags::BlobCustomSerltnWidgetsOnly))
    {
        if (nullptr == deserialize_cni_node(root))
        {
            m_wf->remove_node(m_curr_node);
            LogRError("Dummy CNI node " << node_class, nullptr);
        }
    }

    return m_curr_node;
}

bool WfDeserializerFromBlob::deserialize_pin(blob::Reader& root_block, bool dummy)
{
    blob::Reader root = root_block.pop_block();
    blob::Reader block = root.pop_block();

    const auto def_type_str = block.pop_string_ptr();
    const auto def_type = cast::to_enum<PinType>(block.pop_u32());
    if (dummy)
    {
        m_curr_pin->get_object()->def_state.type_str = def_type_str;
        m_curr_pin->get_object()->def_state.type = def_type;
    }
    else if (m_curr_pin->get_object()->def_state.type_str != def_type_str)
    {
        LogRAssert(m_curr_pin->get_object()->def_state.type_str == def_type_str,
            "Pin must be dummy. w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", p:" << m_curr_pin->get_instance_id() << ", t:" << def_type_str, false);
    }

    m_curr_pin->set_name(block.pop_string());
    m_curr_pin->state().type_str = block.pop_string_ptr();
    m_curr_pin->state().type = cast::to_enum<PinType>(block.pop_u32());

    m_curr_pin->state().value = block.pop_primitive();
    m_curr_pin->state().min = block.pop_primitive();
    m_curr_pin->state().max = block.pop_primitive();
    m_curr_pin->state().step = block.pop_primitive();
    m_curr_pin->state().cag_val = cast::to_enum<ControlAfterGenerate>(block.pop_u8());

    m_curr_pin->flags().add_raw(block.pop_u64());
    m_curr_pin->obj_flags().add_raw(block.pop_u64());

    if (PinType::COMBO == m_curr_pin->state().type && cast::to_szt(m_curr_pin->state().value.get_int()) >= m_curr_pin->get_object()->combo_items.size())
        m_curr_pin->state().value = int64_t(0);

    return true;
}

bool WfDeserializerFromBlob::check_node_definition(blob::Reader& block) const
{
    auto def_block = block.pop_block();

    const bool is_group = def_block.pop_bool();
    if (is_group)
        return true;

    const bool is_cni = def_block.pop_bool();
    const auto node_class = def_block.pop_string();

    if (is_cni)
        return check_cni_node_definition(block, node_class);

    const auto node = g::pNodesManager->find_node(base::HashID::make_hash(node_class));
    if (nullptr == node)
        return false;

    const size_t ins = def_block.pop_u32();
    const size_t outs = def_block.pop_u32();
    if (ins != node->input.size() || outs != node->output.size())
        return false;

    for (size_t i = 0; i < ins; i++)
    {
        const auto type = cast::to_enum<PinType>(def_block.pop_u32());
        if (node->input[i]->def_state.type != type)
            return false;
    }
    for (size_t i = 0; i < outs; i++)
    {
        const auto type = cast::to_enum<PinType>(def_block.pop_u32());
        if (node->output[i]->def_state.type != type)
            return false;
    }

    return true;
}

void WfDeserializerFromBlob::deserialize_link(blob::Reader& root_block)
{
    blob::Reader block = root_block.pop_block();
    const auto id = block.pop_iid();
    const auto p1_id = block.pop_iid();
    const auto p2_id = block.pop_iid();
    auto p1 = m_wf->get_pin_by_instance(p1_id);
    auto p2 = m_wf->get_pin_by_instance(p2_id);
    LogRAssert(p1 != nullptr && p2 != nullptr, "Can't create link - pin(s) not found. w:" << m_wf->get_id()
        << ", l:" << id << ", p1: " << p1_id << ":" << (p1 != nullptr) << ", p2: " << p2_id << ":" << (p2 != nullptr), );
    LogRAssert(Link::link(p1, p2, id), "Can't create link. w:" << m_wf->get_id() << ", l:" << id << ", p1 : " << p1_id << ", p2 : " << p2_id, );
}

} // namespace data
