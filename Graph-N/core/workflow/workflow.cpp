
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/node_instance_ro.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/custom_nodes/all_impl.h"
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

Workflow::Workflow(mngr::WfData&& data)
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

Workflow::~Workflow() noexcept
{
    if (g::pGraphN != nullptr && !m_wf_data.ro)
        g::pGraphN->unsubscribe_comfyui_validity_state(this);
}

void Workflow::init()
{
    m_nodes.reserve(30);
    m_links.reserve(150);
    m_action = std::make_unique<WorkflowAction>(this);
}

std::unique_ptr<NodeInstance> Workflow::create_node(data::Node* node, iid node_id) const
{
    if (node != nullptr)
    {
        std::unique_ptr<NodeInstance> custom_node;
        if (custom_node && custom_node->cni_is_applicable())
        {
            Log("Node created (CNI). w:" << get_id() << ", n:" << custom_node->get_instance_id() << ":" << node->node_class);
            return custom_node;
        }
    }
    auto instance = std::make_unique<NodeInstance>(node, get_id(), node_id);
    Log("Node created. w:" << get_id() << ", n:" << instance->get_instance_id() << ":" << (node != nullptr ? node->node_class : "group"));
    return instance;
}

std::unique_ptr<NodeInstanceRo> Workflow::create_node_ro(data::Node* node, iid node_id) const
{
    if (node != nullptr)
    {
        std::unique_ptr<NodeInstanceRo> custom_node;
        if (custom_node && custom_node->cni_is_applicable())
        {
            Log("Node(RO) created (CNI). w:" << get_id() << ", n:" << custom_node->get_instance_id() << ":" << node->node_class);
            return custom_node;
        }
    }
    auto instance = std::make_unique<NodeInstanceRo>(node, get_id(), node_id);
    Log("Node(RO) created. w:" << get_id() << ", n:" << instance->get_instance_id() << ":" << node->node_class);
    return instance;
}

NodeInstance* Workflow::add_node(data::Node* node, const ImRect& rc /*= ImRect(0, 0, 0, 0)*/, const std::string& select /*= ""*/, iid node_id /*= 0*/)
{
    const auto id = (0 == node_id ? get_new_child_id() : node_id);
    auto instance = create_node(node, id);
    instance->m_rc = rc;
    auto ptr = instance.get();
    ptr->init(select);
    m_nodes.push_back(std::move(instance));
    Log("Node added wid=" << m_wf_data.id << ", hid=" << node->get_id() << ", iid=" << id);
    return ptr;
}

NodeInstance* Workflow::add_node(const NodeInstance* copy_from_source, bool copy_id)
{
    LogRAssert(copy_from_source, "Zero ptr", nullptr);
    LogRAssert(copy_from_source->get_object(), "Zero object ptr", nullptr);
    const auto id = (copy_id ? copy_from_source->get_instance_id() : get_new_child_id());
    auto instance = create_node(copy_from_source->get_object(), id);
    auto ptr = instance.get();
    ptr->init(copy_from_source, copy_id);
    m_nodes.push_back(std::move(instance));
    return ptr;
}

NodeInstance* Workflow::add_group_node()
{
    auto instance = create_node(nullptr, 0);
    instance->init_blank();
    instance->m_group = true;
    auto ptr = instance.get();
    m_nodes.push_back(std::move(instance));
    return ptr;
}

NodeInstanceRo* Workflow::add_node_ro(data::Node* node)
{
    const auto id = get_new_child_id();
    auto instance = create_node_ro(node, id);
    auto ptr = instance.get();
    ptr->init();
    m_nodes_ro.push_back(std::move(instance));
    return ptr;
}

NodeInstance* Workflow::add_group(const ImRect& rc, const std::string& name /*= ""*/)
{
    const auto id = get_new_child_id();
    auto instance = create_node(nullptr, id);
    instance->m_rc.Min = rc.Min;
    instance->m_rc.Max = ImVec2(-1 == rc.Max.x ? 200 : rc.Max.x, -1 == rc.Max.y ? 200 : rc.Max.y);
    instance->m_group = true;
    instance->set_name(name);
    auto ptr = instance.get();
    ptr->init("");
    m_nodes.push_back(std::move(instance));
    return ptr;
}

PinInstance* Workflow::add_pin(NodeInstance* node, data::Pin* pin, iid instance_id /*= 0*/)
{
    LogRAssert(node && pin, "Zero ptr", nullptr);
    const auto id = (0 == instance_id ? get_new_child_id() : instance_id);
    auto instance = std::make_unique<PinInstance>(node, pin, m_wf_data.id, id);
    auto ptr = instance.get();

    Log("Pin created. w:" << get_id()
        << ", n:" << node->get_instance_id() << ":" << node->get_name_original()
        << ", p:" << ptr->get_instance_id() << ", pt:" << pin->def_state.type_str << ", pn:" << pin->def_state.name << ", pi:" << pin->out_index);

    ptr->init();
    m_pins.push_back(std::move(instance));
    return ptr;
}

PinInstance* Workflow::add_pin(NodeInstanceRo* node, data::Pin* pin)
{
    LogRAssert(node && pin, "Zero ptr", nullptr);
    const auto id = get_new_child_id();
    auto instance = std::make_unique<PinInstance>(node, pin, m_wf_data.id, id);
    auto ptr = instance.get();

    Log("Pin(RO) created. w:" << get_id()
        << ", n:" << node->get_instance_id() << ":" << node->get_name_original()
        << ", p:" << ptr->get_instance_id() << ", pt:" << pin->def_state.type_str << ", pn:" << pin->def_state.name << ", pi:" << pin->out_index);

    ptr->init();
    m_pins.push_back(std::move(instance));
    return ptr;
}

Link* Workflow::add_link(iid instance_id /*= 0*/)
{
    const auto id = (0 == instance_id ? get_new_child_id() : instance_id);
    auto link = std::make_unique<Link>(m_wf_data.id, id);
    auto ptr = link.get();
    Log("Link created. w:" << get_id() << ", l:" << link->get_instance_id());
    m_links.push_back(std::move(link));
    return ptr;
}

NodeInstance* Workflow::get_node_by_instance(iid id) const noexcept
{
    LogRAssert(id != 0, "Zero node id", nullptr);
    const auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id](const std::unique_ptr<NodeInstance>& node) noexcept
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

PinInstance* Workflow::get_pin_by_instance(iid id) const noexcept
{
    LogRAssert(id != 0, "Zero pin id", nullptr);
    const auto it = std::find_if(m_pins.begin(), m_pins.end(),
        [id](const std::unique_ptr<PinInstance>& pin) noexcept
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

NodeInstance* Workflow::get_node_by_hash(hid hash) const noexcept
{
    LogRAssert(hash != 0, "Zero node hash", nullptr);
    const auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [hash](const std::unique_ptr<NodeInstance>& node) noexcept
        {
            if (!node || !node->get_object())
                return false;
            return node->get_object()->get_hash() == hash;
        }
    );
    if (it == m_nodes.end())
        return nullptr;
    return (*it).get();
}

PinInstance* Workflow::get_pin_by_hash(iid node_iid, hid pin_hash) const noexcept
{
    LogRAssert(node_iid != 0 && pin_hash != 0, "Zero node/pin id", nullptr);
    const auto it = std::find_if(m_pins.begin(), m_pins.end(),
        [node_iid, pin_hash](const std::unique_ptr<PinInstance>& pin) noexcept
        {
            if (!pin || !pin->get_object())
                return false;
            return (pin->get_object()->get_hash() == pin_hash && pin->get_parent_node() == node_iid);
        }
    );
    if (it == m_pins.end())
        return nullptr;
    return (*it).get();
}

Link* Workflow::get_link_by_instance(iid id) const noexcept
{
    LogRAssert(id != 0, "Zero link id", nullptr);
    for (auto& link : m_links)
    {
        if (link->get_instance_id() == id)
            return link.get();
    }
    return nullptr;
}

std::list<Link*> Workflow::get_links_by_pin(const PinInstance* pin) const
{
    LogRAssert(pin, "Zero ptr", std::list<Link*>{});
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

void Workflow::remove_node(NodeInstance* node)
{
    LogRAssert(node, "Zero ptr", );
    const auto n_it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id = node->get_instance_id()](const std::unique_ptr<NodeInstance>& node) noexcept
        {
            if (!node)
                return false;
            return node->get_instance_id() == id;
        }
    );

    LogRAssert(n_it != m_nodes.end(), "Node not found. w:" << get_id() << ", n:" << node->get_instance_id(), );

    const auto remove_pins = [this](const std::vector<PinInstance*>& pins)
    {
        for (auto pin : pins)
        {
            remove_pin(pin);
        }
    };

    if (node->get_object() != nullptr && node->get_object()->is_dummy)
        g::pNodesManager->remove_dummy_node(node->get_object());

    node->cni_cleanup_base_object();
    remove_pins(node->get_inputs());
    remove_pins(node->get_outputs());

    if (node->get_object() == nullptr) { Log("Group removed. w:" << get_id() << ", n:" << node->get_instance_id()); }
    else { Log("Node removed. w:" << get_id() << ", n:" << node->get_instance_id()); }

    m_nodes.erase(n_it);
    invalidate_current();
}

void Workflow::invalidate_current() noexcept
{
    if (!m_wf_data.valid && !m_wf_data.critical_failure)
    {
        m_wf_data.valid = true;
        for (auto& ptr : m_nodes)
        {
            if (!ptr->m_group && ptr->get_object()->is_dummy)
            {
                m_wf_data.valid = false;
                break;
            }
        }
    }
}

void Workflow::remove_node(NodeInstanceRo* node)
{
    LogRAssert(node, "Zero ptr", );
    const auto n_it = std::find_if(m_nodes_ro.begin(), m_nodes_ro.end(),
        [id = node->get_instance_id()](const std::unique_ptr<NodeInstanceRo>& node) noexcept
        {
            if (!node || !node->get_object())
                return false;
            return node->get_instance_id() == id;
        }
    );

    LogRAssert(n_it != m_nodes_ro.end(), "RoNode not found. w:" << get_id() << ", n:" << node->get_instance_id(), );

    const auto remove_pins = [this](const std::vector<PinInstance*>& pins)
    {
        for (auto pin : pins)
        {
            remove_pin(pin);
        }
    };

    node->cni_cleanup_base_object();
    remove_pins(node->get_inputs());
    remove_pins(node->get_outputs());

    Log("Node(RO) removed. w:" << get_id() << ", n:" << node->get_instance_id());
    m_nodes_ro.erase(n_it);
}

void Workflow::remove_pin(const PinInstance* pin) noexcept
{
    LogRAssert(pin, "Zero ptr", );
    LogRAssert(pin->state().links == 0, "Links must be removed before pin removal. w:" << get_id() << ", p:" << pin->get_instance_id() << ", ls:" << pin->state().links, );

    const auto it = std::find_if(m_pins.begin(), m_pins.end(), [pin](const std::unique_ptr<PinInstance>& ptr) noexcept { return ptr.get() == pin; });
    LogRAssert(it != m_pins.end(), "Pin not found. w:" << get_id() << ", p:" << pin->get_instance_id(), );

    Log("Pin removed. w:" << get_id() << ", n:" << pin->get_parent_node() << ", p:" << pin->get_instance_id() << ", t:" << pin->state().type_str);
    m_pins.erase(it);    
}

void Workflow::remove_link(const Link* link)
{
    LogRAssert(link, "Zero ptr", );
    const auto it = std::find_if(m_links.begin(), m_links.end(),
        [id = link->get_instance_id()](const std::unique_ptr<Link>& link) noexcept
        {
            if (!link)
                return false;
            return link->get_instance_id() == id;
        }
    );
    LogRAssert(it != m_links.end(), "Link not found. w:" << get_id() << ", l:" << link->get_instance_id(), );

    it->get()->unlink();
    Log("Link removed. w:" << get_id() << ", l:" << link->get_instance_id());
    m_links.erase(it);
}


void Workflow::bypass_node(data::NodeInstance* node)
{
    LogRAssert(node, "Zero ptr", );
    
    m_action->multi_action_on("Bypass");

    std::list<std::pair<PinInstance*, PinInstance*>> reroutes;
    auto& links = get_links();
    auto& ins = node->get_inputs();
    auto& outs = node->get_outputs();

    for (size_t i = 0; i < outs.size(); i++)
    {
        auto out = outs[i];
        if (0 == out->state().links)
            continue;

        int32_t out_type_idx = -1;
        for (size_t j = 0; j < outs.size(); j++)
        {
            if (outs[j]->state().type == out->state().type)
            {
                out_type_idx++;

                if (out == outs[j])
                    break;
            }
        }
        LogCAssert(out_type_idx >= 0, "Out index not found. w:" << get_id() << ", n:" << node->get_instance_id() << ", p:" << out->get_instance_id());

        int32_t in_type_idx = -1;
        PinInstance* in = nullptr;
        for (size_t j = 0; j < ins.size(); j++)
        {
            if (ins[j]->state().type == out->state().type)
            {
                in_type_idx++;

                if (0 == in_type_idx || out_type_idx == in_type_idx)
                    in = ins[j];
            }
        }

        if (in != nullptr && in->state().links > 0)
        {
            PinInstance* target_out = nullptr;
            for (auto& link : links)
            {
                if (link->get_input() == in)
                {
                    target_out = link->get_output();
                    break;
                }
            }
            LogCAssert(target_out != nullptr, "Target not found. w:" << get_id() << ", n:" << node->get_instance_id() << ", p:" << in->get_instance_id());

            for (auto& link : links)
            {
                if (link->get_output() == out)
                {
                    reroutes.push_back({ target_out, link->get_input() });
                }
            }
        }
    }

    for (auto in : ins)
    {
        if (in->state().links > 0)
            m_action->unlink(in);
    }
    for (auto out : outs)
    {
        if (out->state().links > 0)
            m_action->unlink(out);
    }

    for (const auto& pair : reroutes)
    {
        LogCAssert(Link::can_link(pair.first, pair.second), "Can't link rerouted pins, w:" << get_id() << ", n:" << node->get_instance_id() << ", p1:" << pair.first->get_instance_id() << ", p2:" << pair.second->get_instance_id());
        m_action->link(pair.first, pair.second);
    }

    m_action->multi_action_off();
}

void Workflow::prepare_for_api()
{
    auto& nodes_ptr = get_nodes();
    std::vector<NodeInstance*> nodes;
    std::for_each(nodes_ptr.begin(), nodes_ptr.end(), [&nodes](const std::unique_ptr<NodeInstance>& node) { nodes.push_back(node.get()); });

    for (auto node : nodes)
    {
        bool remove = (node->m_group || node->get_mode() == NodeMode::Mute);


        if (!remove && node->get_mode() == NodeMode::Bypass)
        {
            bypass_node(node);
            remove = true;
        }

        if (remove || !node->cni_json_api_should_export())
        {
            m_action->delete_node(node);
        }
    }
}

}

void Workflow::reset() noexcept
{
    for (auto& node : m_nodes)
    {
        node->reset_state();

        if (!node->m_group)
            node->reset_width();
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

void Workflow::clear() noexcept
{
    m_nodes.clear(); m_nodes.shrink_to_fit();
    m_nodes_ro.clear(); m_nodes_ro.shrink_to_fit();
    m_pins.clear(); m_pins.shrink_to_fit();
    m_links.clear(); m_links.shrink_to_fit();
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

void Workflow::increment(const Workflow* copy_to)
{
    for (auto& node : m_nodes)
    {
        auto copy_to_node = (nullptr == copy_to ? nullptr : copy_to->get_node_by_instance(node->get_instance_id()));
        auto copy_to_action = (nullptr == copy_to ? nullptr : copy_to->action());
        node->increment(copy_to_node, copy_to_action);
    }
}

std::string Workflow::collect_search_data() const
{
    std::stringstream data;
    for (auto& node : m_nodes)
    {
        node->collect_search_data(data);
        node->cni_collect_search_data(data);
    }
    return data.str();
}

} // namespace data
