
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"
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

Workflow::~Workflow()
{}

void Workflow::init()
{
    LogF;

    m_nodes.reserve(30);
    m_links.reserve(150);

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

NodeInstance* Workflow::add_node(data::Node* node, const std::string& select /*= ""*/)
{
    const auto id = get_new_child_id();
    auto instance = std::make_unique<NodeInstance>(node, m_wf_data.id, id);
    auto ptr = instance.get();
    ptr->init(select);
    m_nodes.push_back(std::move(instance));
    Log("Node added wid=" << m_wf_data.id << ", hid=" << node->get_id() << ", iid=" << id);
    return ptr;
}

PinInstance* Workflow::add_pin(NodeInstance* node, data::Pin* pin)
{
    const auto id = get_new_child_id();
    auto instance = std::make_unique<PinInstance>(node->get_instance_id(), pin, m_wf_data.id, id);
    auto ptr = instance.get();
    ptr->init();
    m_pins[ptr->get_instance_id()] = std::move(instance);
    Log("Pin added wid="
        << m_wf_data.id << ", node hid=" << node->get_object()->get_id() << ", node iid=" << node->get_instance_id()
        << ", pin hid=" << pin->get_id() << ", pin iid=" << ptr->get_instance_id());
    return ptr;
}

Link* Workflow::add_link()
{
    const auto id = get_new_child_id();
    auto link = std::make_unique<Link>(m_wf_data.id, id);
    auto ptr = link.get();
    m_links.push_back(std::move(link));
    Log("Node added wid=" << m_wf_data.id << ", iid=" << id);
    return ptr;
}

NodeInstance* Workflow::get_node(iid id) const
{
    const auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id](const std::unique_ptr<NodeInstance>& node)
        {
            if (!node || !node->get_object())
                return false;
            return node->get_instance_id() == id;
        }
    );
    if (it == m_nodes.end())
        return nullptr;
    return (*it).get();
}

PinInstance* Workflow::get_pin(iid id) const
{
    const auto it = m_pins.find(id);
    if (it == m_pins.end())
        return false;
    return it->second.get();
}

void Workflow::remove_node(const NodeInstance* node)
{
    const auto n_it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id = node->get_instance_id()](const std::unique_ptr<NodeInstance>& node)
        {
            if (!node || !node->get_object())
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
        for (auto& pin : pins)
        {
            remove_pin(pin);
        }
    };

    remove_pins(node->m_input);
    remove_pins(node->m_output);

    //TODO
//    if (!m_invalidation_state)
        m_nodes.erase(n_it);
}

void Workflow::remove_pin(const PinInstance* pin)
{
    const auto p_it = m_pins.find(pin->get_instance_id());
    if (p_it == m_pins.end())
    {
        LogInternalMap("Pin " << pin->get_instance_id() << " not found");
    }

    const auto remove_links = [this](const PinInstance* pin)
    {
        for (auto l_it = m_links.begin(); l_it != m_links.end(); )
        {
            auto link = l_it->get();
            if (link->get_input() == pin || link->get_output() == pin)
            {
                link->unlink();
                l_it = m_links.erase(l_it);

                if (pin->m_state->links == 0)
                    break;
            }
            else
                ++l_it;
        }
    };

    if (pin->m_state->links > 0)
        remove_links(pin);

    //TODO
   // if (!m_invalidation_state)
        m_pins.erase(p_it);
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
