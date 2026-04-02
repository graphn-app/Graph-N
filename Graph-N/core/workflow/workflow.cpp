
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/node_instance_ro.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/workflow_action.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/workflows_manager.h"

namespace data
{

Workflow::Workflow()
{
    m_wf_data.uuid = base::GlobalID::uuid();
    m_wf_data.id = tools::str::crc32(m_wf_data.uuid);
    subscribe_comfyui_validity_state();
}

Workflow::Workflow(WfData&& data)
{
    m_wf_data = std::move(data);
    if (m_wf_data.uuid.empty())
        m_wf_data.uuid = base::GlobalID::uuid();
    m_wf_data.id = tools::str::crc32(m_wf_data.uuid);
    subscribe_comfyui_validity_state();
}

Workflow::Workflow(const char* uuid, const char* name)
{
    m_wf_data.uuid = uuid;
    m_wf_data.id = tools::str::crc32(m_wf_data.uuid);
    m_wf_data.name = name;
    subscribe_comfyui_validity_state();
}

Workflow::~Workflow()
{
    if (g::pGraphN != nullptr && !m_wf_data.ro)
        g::pGraphN->unsubscribe_comfyui_validity_state(this);
}

void Workflow::init()
{
    LogF;

    m_nodes.reserve(30);
    m_links.reserve(150);

    m_action = std::make_unique<WorkflowAction>(this);
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

NodeInstance* Workflow::add_blank_node()
{
    auto instance = std::make_unique<NodeInstance>(nullptr, m_wf_data.id, 0);
    auto ptr = instance.get();
    m_nodes.push_back(std::move(instance));
    Log("Blank node added wid=" << m_wf_data.id);
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
    auto instance = std::make_unique<PinInstance>(node, pin, m_wf_data.id, id);
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
    auto instance = std::make_unique<PinInstance>(node, pin, m_wf_data.id, id);
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

    m_nodes.erase(n_it);
    invalidate_current();
}

void Workflow::invalidate_current()
{
    if (!m_wf_data.valid && !m_wf_data.critical_failure)
    {
        m_wf_data.valid = true;
        for (auto& ptr : m_nodes)
        {
            if (ptr->get_object()->is_dummy)
            {
                m_wf_data.valid = false;
                break;
            }
        }
    }
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

void Workflow::reset()
{
    for (auto& node : m_nodes)
    {
        node->reset_state();

        if (!node->m_group)
            node->calc_node_width(false, false);
    }

    for (auto& link : m_links)
    {
        link->reset_state();
    }

    for (auto& pin : m_pins)
    {
        pin->reset_state();
    }
}

void Workflow::clear()
{
    m_nodes.clear();
    m_nodes_ro.clear();
    m_pins.clear();
    m_links.clear();
}

void Workflow::subscribe_comfyui_validity_state()
{
    if (!m_wf_data.ro)
    {
        g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::Invalidate, this);
        g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::ResolveWfs, this);
    }
}

void Workflow::invalidate_comfyui_objects()
{
    if (m_wf_data.ro || !m_wf_data.activated)
    {
        clear();
        return;
    }

    m_invalidation_state = true;
    g::pWfsManager->dump(m_wf_data.id);
    clear();
}

void Workflow::resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step /*step*/)
{
    if (m_wf_data.ro || !m_wf_data.activated)
        return;

    blob::Blob drawer_state;
    if (g::pWfsManager->load(m_wf_data.id, drawer_state))
        m_action->reinit(std::move(drawer_state));

    m_invalidation_state = false;
}

} // namespace data
