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
        m_wf->clear(); 

        blob::Reader root = m_reader.pop_block();

        blob::Reader block = root.pop_block();
        m_wf->set_last_child_id(block.pop_iid());

        block = root.pop_block();
        drawer_state = block.pop_blob();

        block = root.pop_block();
        const uint32_t nodes_count = block.pop_u32();
        for (uint32_t i = 0; i < nodes_count; i++)
        {
            const auto* node = deserialize_node(block, false);
            if (!node->m_group && node->get_object()->is_dummy)
                data.valid = false;
        }

        block = root.pop_block();
        const uint32_t links_count = block.pop_u32();
        for (uint32_t i = 0; i < links_count; i++)
        {
            deserialize_link(block);
        }

        auto& nodes = m_wf->get_nodes();
        for (auto& node : nodes)
        {
            node->cni_deserialilzation_ends();
        }

#ifdef _DEBUG
        int32_t dup_errs = 0;
        std::set<iid> ids;

        for (auto& node : nodes)
        {
            if (node->m_group)
                continue;

            if (!ids.insert(node->get_instance_id()).second)
            {
                if (0 == dup_errs)
                    LogError("Duplicate node id. w:" << m_wf->get_id() << ", n:" << node->get_instance_id());
                dup_errs++;
            }
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
    auto node = deserialize_node(block, false);
    if (node != nullptr)
        node->cni_deserialilzation_ends();
    return node;
}

NodeInstance* WfDeserializerFromBlob::deserialize_node(blob::Reader& root_block, bool dummy)
{
    blob::Reader reader_copy = root_block.clone();
    blob::Reader root = root_block.pop_block();

    blob::Reader block = root.pop_block();
    const auto obj_hid = block.pop_hid();
    const auto obj_iid = block.pop_iid();
    const auto node_class = block.pop_string();
    const auto node_name = block.pop_string();

    m_curr_node = nullptr;
    if (0 == obj_hid)
        m_curr_node = m_wf->add_group_node();
    else
    {
        m_curr_node = m_wf->add_node(g::pNodesManager->find_node(obj_hid), {}, {}, obj_iid);
        m_curr_node->cni_deserialilzation_begins();

        if (m_curr_node->get_object() != nullptr)
        {
            LogRAssert(!m_curr_node->get_object()->is_dummy, "Dummy node " << node_class << " in base map", nullptr);
        }
        if (nullptr == m_curr_node->get_object() || dummy)
        {
            LogWarn("Node " << node_class << " not found");
            m_curr_node->set_object(g::pNodesManager->make_dummy_node(std::string(node_class)));
            m_curr_node->reset_pins();
        }
    }

    m_curr_node->set_name(std::string(node_name));
    m_curr_node->m_rc = block.pop_rect();
    m_curr_node->m_clr_node = block.pop_vec4();
    m_curr_node->m_clr_group = block.pop_clr();
    m_curr_node->m_clr_head = block.pop_clr();
    m_curr_node->m_clr_group_capt = block.pop_clr();
    m_curr_node->m_group = block.pop_bool();
    m_curr_node->m_show_hidden = block.pop_bool();
    m_curr_node->expand(block.pop_bool());
    m_curr_node->pin(block.pop_bool());
    m_curr_node->set_mode(cast::to_enum<NodeMode>(block.pop_u32()));

    //BREAK_IF(node_name == "Negative");

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

    if (!dummy && m_curr_node->is_cni())
    {
        if (nullptr == deserialize_cni_node(root))
        {
            m_wf->remove_node(m_curr_node);
            return deserialize_node(reader_copy, true);
        }
        m_curr_node->link_cag_pins();
        return m_curr_node;
    }

    const auto unpack_pins = [this](blob::Reader block, bool input_pin)
    {
        const auto pin_count = cast::to_i32(block.pop_u32());
        for (int32_t idx = 0; idx < pin_count; idx++)
        {
            const auto h_id = block.pop_hid();
            const auto i_id = block.pop_iid();
            const auto def_type = block.pop_string();
            const auto def_name = block.pop_string();
            //auto pin = m_curr_node->get_object()->find_pin(h_id, input_pin);
            auto pin = g::pNodesManager->find_pin(h_id);
            if (m_curr_node->get_object()->is_dummy || nullptr == pin || pin->def_state.type_str != def_type)
            {
                if (!m_curr_node->get_object()->is_dummy)
                    return false;
                pin = g::pNodesManager->make_dummy_pin(m_curr_node->get_object(), std::string(def_type), std::string(def_name), input_pin ? -1 : idx);
            }
            if (!m_curr_node->get_object()->is_dummy)
            {
                LogRAssertS(pin->get_hash() == h_id, "Pin ids don't match. w:" << m_curr_node->get_workflow_id() << ", n:" << m_curr_node->get_instance_id() << ", t:" << pin->def_state.type_str << ", ph1:" << pin->get_hash() << ", ph2:" << h_id, false);
            }

            m_curr_pin = m_wf->add_pin(m_curr_node, pin, i_id);
            deserialize_pin(block, m_curr_node->get_object()->is_dummy);
            if (input_pin)
                m_curr_node->add_input(m_curr_pin);
            else
                m_curr_node->add_output(m_curr_pin);
            //m_curr_pin = nullptr;
        }
        return true;
    };

    if (!unpack_pins(root.pop_block(), true) || !unpack_pins(root.pop_block(), false))
    {
        m_wf->remove_node(m_curr_node);
        return deserialize_node(reader_copy, true);
    }

    m_curr_node->link_cag_pins();
    return m_curr_node;
}

void WfDeserializerFromBlob::deserialize_pin(blob::Reader& root_block, bool dummy)
{
    blob::Reader root = root_block.pop_block();
    blob::Reader block = root.pop_block();

    const auto def_type_str = block.pop_string();
    const auto def_type = cast::to_enum<PinType>(block.pop_u32());
    if (dummy)
    {
        m_curr_pin->get_object()->def_state.type_str = def_type_str;
        m_curr_pin->get_object()->def_state.type = def_type;
    }
    else if (m_curr_pin->get_object()->def_state.type_str != def_type_str)
    {
        LogRAssert(m_curr_pin->get_object()->def_state.type_str == def_type_str,
            "Pin must be dummy. w:" << m_wf->get_id() << ", n:" << m_curr_node->get_instance_id() << ", p:" << m_curr_pin->get_instance_id() << ", t:" << def_type_str, );
    }

    m_curr_pin->set_name(std::string(block.pop_string()));
    m_curr_pin->state().type_str = block.pop_string();
    m_curr_pin->state().type = cast::to_enum<PinType>(block.pop_u32());
    m_curr_pin->state().value = block.pop_primitive();
    m_curr_pin->state().cag = block.pop_iid();
    m_curr_pin->get_object()->cag_require = block.pop_bool();
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
