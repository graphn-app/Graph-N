
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
    m_nodes.reserve(30);
    m_pins.reserve(70);
    m_links.reserve(150);
}

Workflow::~Workflow()
{}

void Workflow::init()
{
    LogF;
    g::pFlow->subscribe_comfyui_validity_state(this);
}
//
//NodeInstance* Workflow::find_node(gid node_id) const
//{
//    const auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
//        [node_id](const std::unique_ptr<NodeInstance>& node)
//        {
//            if (!node || !node->get_object())
//                return false;
//            return node->get_graph_id() == node_id;
//        }
//    );
//
//    if (it == m_nodes.end())
//        return nullptr;
//    return (*it).get();
//}
//
//PinInstance* Workflow::find_pin(gid pin_id) const
//{
//    const auto it = std::find_if(m_pins.begin(), m_pins.end(),
//        [pin_id](const std::unique_ptr<PinInstance>& pin)
//        {
//            if (!pin || !pin->get_object())
//                return false;
//            return pin->get_graph_id() == pin_id;
//        }
//    );
//}
//
//Link* Workflow::find_link(gid link_id) const
//{
//    const auto it = std::find_if(m_links.begin(), m_links.end(),
//        [link_id](const std::unique_ptr<Link>& link)
//        {
//            if (!link)
//                return false;
//            return link->get_graph_id() == link_id;
//        }
//    );
//}

NodeInstance* Workflow::add_node(data::Node* node)
{
    const gid big_id = get_id();
    const iid instance_id = ++m_child_id;
    const gid graph_id = (big_id << 32) | instance_id;

    auto instance = std::make_unique<NodeInstance>(this, node, instance_id, graph_id);
    auto ptr = instance.get();
    ptr->init();
    m_nodes.push_back(std::move(instance));
    Log("Node added hid=" << node->get_id() << ", iid=" << instance_id << ", gid=" << graph_id);
    return ptr;
}

PinInstance* Workflow::add_pin(data::Pin* pin)
{
    const iid instance_id = ++m_child_id;
    const gid big_id = get_id();
    const gid graph_id = (big_id << 32) | instance_id;

    auto instance = std::make_unique<PinInstance>(this, pin, instance_id, graph_id);
    auto ptr = instance.get();
    ptr->init();
    m_pins.push_back(std::move(instance));
    Log("Pin added hid=" << pin->get_id() << ", iid=" << instance_id << ", gid=" << graph_id);
    return ptr;
}

Link* Workflow::add_link()
{
    const gid big_id = get_id();
    const gid graph_id = (big_id << 32) | (++m_child_id);

    auto link = std::make_unique<Link>(this, graph_id);
    auto ptr = link.get();
    m_links.push_back(std::move(link));
    Log("Link added gid=" << link->get_graph_id());
    return ptr;
}

void Workflow::remove_node(const NodeInstance* node)
{
    const auto n_it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id = node->get_graph_id()](const std::unique_ptr<NodeInstance>& node)
        {
            if (!node || !node->get_object())
                return false;
            return node->get_graph_id() == id;
        }
    );

    if (n_it == m_nodes.end())
    {
        LogAndThrowInternalError(ExcepCategory::inconsistent_map, "Node " << node->get_graph_id() << " not found");
    }

    const auto remove_pins = [this](const std::vector<PinInstance*>& pins)
    {
        for (auto& pin : pins)
        {
            remove_pin(pin);
        }
    };

    remove_pins(node->get_input());
    remove_pins(node->get_output());

    if (!m_invalidation_state)
        m_nodes.erase(n_it);
}

void Workflow::remove_pin(const PinInstance* pin)
{
    const auto p_it = std::find_if(m_pins.begin(), m_pins.end(),
        [id = pin->get_graph_id()](const std::unique_ptr<PinInstance>& pin)
        {
            if (!pin || !pin->get_object())
                return false;
            return pin->get_graph_id() == id;
        }
    );
    if (p_it == m_pins.end())
    {
        LogAndThrowInternalError(ExcepCategory::inconsistent_map, "Pin " << pin->get_graph_id() << " not found");
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

                if (pin->get_state()->links == 0)
                    break;
            }
            else
                ++l_it;
        }
    };

    if (pin->get_state()->links > 0)
        remove_links(pin);

    if (!m_invalidation_state)
        m_pins.erase(p_it);
}

void Workflow::remove_link(const Link* link)
{
    const auto it = std::find_if(m_links.begin(), m_links.end(),
        [id = link->get_graph_id()](const std::unique_ptr<Link>& link)
        {
            if (!link)
                return false;
            return link->get_graph_id() == id;
        }
    );
    if (it != m_links.end())
    {
        it->get()->unlink();
        m_links.erase(it);
    }
    else
    {
        LogAndThrowInternalError(ExcepCategory::inconsistent_map, "Link " << link->get_graph_id() << " not found");
    }
}

void Workflow::mark_invalid(hid id)
{
    Log("Obj to remove id=" << id);
    m_invalid_items.insert(id);
}

void Workflow::finalize_resolve_comfyui_objects()
{
    m_invalidation_state = true;
    for (auto id : m_invalid_items)
    {
        for (auto n_it = m_nodes.begin(); n_it != m_nodes.end(); )
        {
            if ((*n_it)->get_object()->get_id() == id)
            {
                remove_node((*n_it).get());
                n_it = m_nodes.erase(n_it);
            }
            else
                ++n_it;
        }

        for (auto p_it = m_pins.begin(); p_it != m_pins.end(); )
        {
            if ((*p_it)->get_object()->get_id() == id)
            {
                remove_pin((*p_it).get());                
                p_it = m_pins.erase(p_it);
            }
            else
                ++p_it;
        }
    }
    m_invalid_items.clear();
    m_invalidation_state = false;
}

} // namespace data
