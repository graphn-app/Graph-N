
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/node_instance_ro.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/workflow_action.h"
#include "core/managers/nodes_manager.h"
#include "tools/blob_serializer.h"
#include "ui/graph-n_flow.h"

namespace data
{

Workflow::Workflow()
{
    m_wf_data.uuid = base::GlobalID::uuid();
    m_wf_data.id = tools::str::crc32(m_wf_data.uuid);
}

Workflow::Workflow(WfData&& data)
{
    m_wf_data = std::move(data);
    if (m_wf_data.uuid.empty())
        m_wf_data.uuid = base::GlobalID::uuid();
    m_wf_data.id = tools::str::crc32(m_wf_data.uuid);
}

Workflow::Workflow(const char* uuid, const char* name)
{
    m_wf_data.uuid = uuid;
    m_wf_data.id = tools::str::crc32(m_wf_data.uuid);
    m_wf_data.name = name;
}

Workflow::~Workflow()
{}

void Workflow::init()
{
    LogF;

    m_nodes.reserve(30);
    m_links.reserve(150);

    m_action = std::make_unique<WorkflowAction>(this);

    //TODO
    //g::pFlow->subscribe_comfyui_validity_state(this);
    Log("Wf '" << m_wf_data.name << "' created, wid=" << m_wf_data.id << ", uuid=" << m_wf_data.uuid)
}

void Workflow::stop_all_threads()
{}

iid Workflow::get_new_child_id()
{
    return ++m_last_child_id;
}

NodeInstance* Workflow::add_node(data::Node* node, const ImRect& rc /*= ImRect(0, 0, 0, 0)*/, const std::string& select /*= ""*/)
{
    const auto id = get_new_child_id();
    auto instance = std::make_unique<NodeInstance>(node, m_wf_data.id, id);
    instance->m_rc = rc;
    auto ptr = instance.get();
    ptr->init(select);
    m_nodes.push_back(std::move(instance));
    Log("Node added wid=" << m_wf_data.id << ", hid=" << node->get_id() << ", iid=" << id);
    return ptr;
}

NodeInstance* Workflow::add_node(const NodeInstance* copy_from_source)
{
    const auto id = get_new_child_id();
    auto instance = std::make_unique<NodeInstance>(copy_from_source->get_object(), m_wf_data.id, id);
    auto ptr = instance.get();
    ptr->init(copy_from_source);
    m_nodes.push_back(std::move(instance));
    if (copy_from_source->m_group)
    {
        Log("Group added wid=" << m_wf_data.id << ", iid=" << id);
    }
    else
    {
        Log("Node added wid=" << m_wf_data.id << ", hid=" << copy_from_source->get_object()->get_id() << ", iid=" << id);
    }
    return ptr;
}

NodeInstance* Workflow::add_node(blob::Reader& reader)
{
    auto instance = std::make_unique<NodeInstance>(nullptr, m_wf_data.id, 0);
    auto ptr = instance.get();
    ptr->init(reader);
    m_nodes.push_back(std::move(instance));
    if (ptr->m_group)
    {
        Log("Group added wid=" << m_wf_data.id << ", iid=" << ptr->get_instance_id());
    }
    else
    {
        Log("Node added wid=" << m_wf_data.id << ", hid=" << ptr->get_object()->get_id() << ", iid=" << ptr->get_instance_id());
    }
    return ptr;
}

NodeInstanceRo* Workflow::add_node_ro(data::Node* node)
{
    const auto id = get_new_child_id();
    auto instance = std::make_unique<NodeInstanceRo>(node, m_wf_data.id, id);
    auto ptr = instance.get();
    ptr->init();
    m_nodes_ro.push_back(std::move(instance));
    Log("RoNode added wid=" << m_wf_data.id << ", hid=" << node->get_id() << ", iid=" << id);
    return ptr;
}

NodeInstance* Workflow::add_group(const ImRect& rc, const std::string& name /*= ""*/)
{
    const auto id = get_new_child_id();
    auto instance = std::make_unique<NodeInstance>(nullptr, m_wf_data.id, id);
    instance->m_rc.Min = rc.Min;
    instance->m_rc.Max = ImVec2(-1 == rc.Max.x ? 200 : rc.Max.x, -1 == rc.Max.y ? 200 : rc.Max.y);
    instance->m_group = true;
    instance->set_name(name);
    auto ptr = instance.get();
    ptr->init("");
    m_nodes.push_back(std::move(instance));
    Log("Group added wid=" << m_wf_data.id << ", iid=" << id);
    return ptr;
}

PinInstance* Workflow::add_pin(NodeInstance* node, data::Pin* pin, iid instance_id /*= 0*/)
{
    const auto id = (0 == instance_id ? get_new_child_id() : instance_id);
    auto instance = std::make_unique<PinInstance>(node->get_instance_id(), pin, m_wf_data.id, id);
    auto ptr = instance.get();
    ptr->init();
    m_pins.push_back(std::move(instance));
    Log("Pin added wid="
        << m_wf_data.id << ", node hid=" << node->get_object()->get_id() << ", node iid=" << node->get_instance_id()
        << ", pin hid=" << pin->get_id() << ", pin iid=" << ptr->get_instance_id());
    return ptr;
}

PinInstance* Workflow::add_pin(NodeInstanceRo* node, data::Pin* pin)
{
    const auto id = get_new_child_id();
    auto instance = std::make_unique<PinInstance>(node->get_instance_id(), pin, m_wf_data.id, id);
    auto ptr = instance.get();
    ptr->init();
    m_pins.push_back(std::move(instance));
    Log("RO Pin added wid="
        << m_wf_data.id << ", node hid=" << node->get_object()->get_id() << ", node iid=" << node->get_instance_id()
        << ", pin hid=" << pin->get_id() << ", pin iid=" << ptr->get_instance_id());
    return ptr;
}

Link* Workflow::add_link(iid instance_id /*= 0*/)
{
    const auto id = (0 == instance_id ? get_new_child_id() : instance_id);
    auto link = std::make_unique<Link>(m_wf_data.id, id);
    auto ptr = link.get();
    m_links.push_back(std::move(link));
    Log("Link added wid=" << m_wf_data.id << ", iid=" << id);
    return ptr;
}

NodeInstance* Workflow::get_node_by_instance(iid id) const
{
    const auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id](const std::unique_ptr<NodeInstance>& node)
        {
            if (!node)
                return false;
            return node->get_instance_id() == id;
        }
    );
    if (it == m_nodes.end())
        return nullptr;
    return (*it).get();
}

PinInstance* Workflow::get_pin_by_instance(iid id) const
{
    const auto it = std::find_if(m_pins.begin(), m_pins.end(),
        [id](const std::unique_ptr<PinInstance>& pin)
    {
        if (!pin || !pin->get_object())
            return false;
        return pin->get_instance_id() == id;
    }
    );
    if (it == m_pins.end())
        return nullptr;
    return (*it).get();
}

NodeInstance* Workflow::get_node_by_hash(hid id) const
{
    const auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id](const std::unique_ptr<NodeInstance>& node)
        {
            if (!node || !node->get_object())
                return false;
            return node->get_object()->get_id() == id;
        }
    );
    if (it == m_nodes.end())
        return nullptr;
    return (*it).get();
}

PinInstance* Workflow::get_pin_by_hash(hid id) const
{
    const auto it = std::find_if(m_pins.begin(), m_pins.end(),
        [id](const std::unique_ptr<PinInstance>& pin)
        {
            if (!pin || !pin->get_object())
                return false;
            return pin->get_object()->get_id() == id;
        }
    );
    if (it == m_pins.end())
        return nullptr;
    return (*it).get();
}

Link* Workflow::get_link_by_instance(iid id) const
{
    for (auto& link : m_links)
    {
        if (link->get_instance_id() == id)
            return link.get();
    }
    return nullptr;
}

std::list<Link*> Workflow::get_links_by_pin(const PinInstance* pin) const
{
    std::list<Link*> ret;
    for (auto& link : m_links)
    {
        if (link->get_input() == pin || link->get_output() == pin)
        {
            ret.push_back(link.get());
        }
    }
    return ret;
}

void Workflow::remove_node(const NodeInstance* node)
{
    const auto n_it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id = node->get_instance_id()](const std::unique_ptr<NodeInstance>& node)
        {
            if (!node)
                return false;
            return node->get_instance_id() == id;
        }
    );

    if (n_it == m_nodes.end())
    {
        LogInternalMap("Node " << node->get_instance_id() << " not found");
    }

    const auto remove_pins = [this](const std::vector<PinInstance*>& pins)
    {
        for (auto pin : pins)
        {
            remove_pin(pin);
        }
    };

    remove_pins(node->get_inputs());
    remove_pins(node->get_outputs());

    //TODO
//    if (!m_invalidation_state)
    m_nodes.erase(n_it);
}

void Workflow::remove_node(const NodeInstanceRo* node)
{
    const auto n_it = std::find_if(m_nodes_ro.begin(), m_nodes_ro.end(),
        [id = node->get_instance_id()](const std::unique_ptr<NodeInstanceRo>& node)
        {
            if (!node || !node->get_object())
                return false;
            return node->get_instance_id() == id;
        }
    );

    if (n_it == m_nodes_ro.end())
    {
        LogInternalMap("Node " << node->get_instance_id() << " not found");
    }

    const auto remove_pins = [this](const std::vector<PinInstance*>& pins)
    {
        for (auto pin : pins)
        {
            remove_pin(pin);
        }
    };

    remove_pins(node->get_inputs());
    remove_pins(node->get_outputs());

    m_nodes_ro.erase(n_it);
}

void Workflow::remove_pin(const PinInstance* pin)
{
    if (pin->state().links > 0)
    {
        LogInternalLogic("Links must be removed before pin delete");
    }

    auto it = std::find_if(m_pins.begin(), m_pins.end(), [pin](const std::unique_ptr<PinInstance>& ptr) { return ptr.get() == pin; });
    if (it == m_pins.end())
    {
        LogInternalLogic("Pin " << pin->get_instance_id() << " not found");
    }
    else
        m_pins.erase(it);

    //TODO
   // if (!m_invalidation_state)
}

void Workflow::remove_link(const Link* link)
{
    const auto it = std::find_if(m_links.begin(), m_links.end(),
        [id = link->get_instance_id()](const std::unique_ptr<Link>& link)
        {
            if (!link)
                return false;
            return link->get_instance_id() == id;
        }
    );
    if (it != m_links.end())
    {
        it->get()->unlink();
        m_links.erase(it);
    }
    else
    {
        LogInternalMap("Link " << link->get_instance_id() << " not found");
    }
}

blob::Writer Workflow::serialize() const
{
    blob::Writer writer;
    blob::Block block(writer);
    writer
        .push(m_last_child_id)
    ;
    block.end();

    writer.push(cast::to_u32(m_nodes.size()));
    for (auto& node : m_nodes)
    {
        block.begin();
        node->serialize(writer);
        block.end();
    }

    writer.push(cast::to_u32(m_links.size()));
    for (auto& link : m_links)
    {
        block.begin();
        link->serialize(writer);
        block.end();
    }

    return writer;
}

std::unique_ptr<Workflow> Workflow::deserialize(const char* uuid, const char* name, const blob::Blob& blob)
{
    try
    {
        blob::Reader wf_reader(blob);
        blob::Reader block = wf_reader.pop_block();

        std::unique_ptr<Workflow> wf = std::make_unique<Workflow>(uuid, name);
        wf->m_last_child_id = block.pop_iid();
        wf->init();

        const uint32_t nodes_count = wf_reader.pop_u32();
        for (uint32_t i = 0; i < nodes_count; i++)
        {
            block = wf_reader.pop_block();
            wf->add_node(block);
        }

        const uint32_t links_count = wf_reader.pop_u32();
        for (uint32_t i = 0; i < links_count; i++)
        {
            block = wf_reader.pop_block();
            Link::deserialize(wf.get(), block);
        }
        return wf;
    }
    catch (std::exception& ex)
    {
        LogExcepS(ExcepCategory::Logic, ex.what());
    }
    return nullptr;
}

//TODO
//void Workflow::mark_invalid(hid id)
//{
//    Log("Obj to remove id=" << id);
//    m_invalid_items.insert(id);
//}

// TODO
//void Workflow::finalize_resolve_comfyui_objects()
//{
//    m_invalidation_state = true;
    

    //for (auto id : m_invalid_items)
    //{
    //    for (auto n_it = m_nodes.begin(); n_it != m_nodes.end(); )
    //    {
    //        if ((*n_it)->get_object()->get_id() == id)
    //        {
    //            remove_node((*n_it).get());
    //            n_it = m_nodes.erase(n_it);
    //        }
    //        else
    //            ++n_it;
    //    }

    //    for (auto p_it = m_pins.begin(); p_it != m_pins.end(); )
    //    {
    //        if ((*p_it)->get_object()->get_id() == id)
    //        {
    //            remove_pin((*p_it).get());                
    //            p_it = m_pins.erase(p_it);
    //        }
    //        else
    //            ++p_it;
    //    }
    //}
    //m_invalid_items.clear();
//    m_invalidation_state = false;
//}

} // namespace data
