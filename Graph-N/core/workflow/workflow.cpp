
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/Pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"
#include "ui/graph-n_flow.h"

namespace data
{

Workflow::Workflow()
{
    m_wf_data.uuid = base::GlobalID::uuid();
    m_id_crc32 = tools::str::crc32(m_wf_data.uuid);

    m_nodes.reserve(30);
    m_pins.reserve(70);
    m_links.reserve(150);
}

Workflow::Workflow(WfData&& data)
    : Workflow()
{
    m_wf_data = std::move(data);
    m_id_crc32 = tools::str::crc32(m_wf_data.uuid);
}

Workflow::~Workflow()
{}

void Workflow::init()
{
    LogF;

    g::pFlow->subscribe_comfyui_validity_state(this);
}

void Workflow::stop_all_threads()
{}

std::pair<gid, iid> Workflow::get_new_child_id()
{
    const iid child_id = ++m_child_id;
    const gid graph_id = (uint64_t(m_id_crc32) << 32) | child_id;
    return std::make_pair(graph_id, child_id);
}

uint32_t Workflow::get_wf_id_by_child_gid(gid child_id)
{
    return cast::to_u32(child_id >> 32);
}

uint32_t Workflow::get_child_id_by_child_gid(gid child_id)
{
    return cast::to_u32(child_id); // just cut
}

NodeInstance* Workflow::add_node(data::Node* node)
{
    const auto id = get_new_child_id();
    auto instance = std::make_unique<NodeInstance>(this, node, id.second, id.first);
    auto ptr = instance.get();
    ptr->init();
    m_nodes.push_back(std::move(instance));
    Log("Node added hid=" << node->get_id() << ", iid=" << id.second << ", gid=" << id.first);
    return ptr;
}

PinInstance* Workflow::add_pin(data::Pin* pin)
{
    const auto id = get_new_child_id();
    auto instance = std::make_unique<PinInstance>(this, pin, id.second, id.first);
    auto ptr = instance.get();
    ptr->init();
    m_pins.push_back(std::move(instance));
    Log("Pin added hid=" << pin->get_id() << ", iid=" << id.second << ", gid=" << id.first);
    return ptr;
}

Link* Workflow::add_link()
{
    const auto id = get_new_child_id();
    auto link = std::make_unique<Link>(this, id.first);
    auto ptr = link.get();
    m_links.push_back(std::move(link));
    Log("Link added gid=" << id.first);
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
        LogInternalMap("Node " << node->get_graph_id() << " not found");
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
        LogInternalMap("Pin " << pin->get_graph_id() << " not found");
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
        LogInternalMap("Link " << link->get_graph_id() << " not found");
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

void Workflow::save()
{}

} // namespace data
