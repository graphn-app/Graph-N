
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
        if (CNI_LoadImage::cni_is_same(node))              custom_node = std::make_unique<CNI_LoadImage>(node, get_id(), node_id);
        else if (CNI_Primitive::cni_is_same(node))         custom_node = std::make_unique<CNI_Primitive>(node, get_id(), node_id);
        else if (CNI_Reroute::cni_is_same(node))           custom_node = std::make_unique<CNI_Reroute>(node, get_id(), node_id);
        else if (CNI_Note::cni_is_same(node))              custom_node = std::make_unique<CNI_Note>(node, get_id(), node_id);
        else if (CNI_MarkdownNote::cni_is_same(node))      custom_node = std::make_unique<CNI_MarkdownNote>(node, get_id(), node_id);

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
        if (CNI_LoadImage::cni_is_same(node))              custom_node = std::make_unique<CNI_LoadImage>(node, get_id(), node_id);
        else if (CNI_Primitive::cni_is_same(node))         custom_node = std::make_unique<CNI_Primitive>(node, get_id(), node_id);
        else if (CNI_Reroute::cni_is_same(node))           custom_node = std::make_unique<CNI_Reroute>(node, get_id(), node_id);
        else if (CNI_Note::cni_is_same(node))              custom_node = std::make_unique<CNI_Note>(node, get_id(), node_id);
        else if (CNI_MarkdownNote::cni_is_same(node))      custom_node = std::make_unique<CNI_MarkdownNote>(node, get_id(), node_id);

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

NodeInstance* Workflow::add_node(data::Node* node, const ImRect& rc /*= ImRect(0, 0, 0, 0)*/, const std::string& select /*= ""*/, iid node_id /*= 0*/, bool dummy /*= false*/)
{
    const auto id = (0 == node_id ? get_new_child_id() : node_id);
    std::unique_ptr<NodeInstance> instance;
    if (dummy)
    {
        instance = std::make_unique<NodeInstance>(node, get_id(), id);
        Log("Dummy node created. w:" << get_id() << ", n:" << instance->get_instance_id() << ":" << node->node_class);
    }
    else
        instance = create_node(node, id);
    instance->m_rc = rc;
    auto ptr = instance.get();
    ptr->init(select);
    m_nodes.push_back(std::move(instance));
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
    const auto it = std::ranges::find_if(m_nodes,
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
    const auto it = std::ranges::find_if(m_pins,
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
    const auto it = std::ranges::find_if(m_nodes,
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
    const auto it = std::ranges::find_if(m_pins,
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
    const auto n_it = std::ranges::find_if(m_nodes,
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
    const auto n_it = std::ranges::find_if(m_nodes_ro,
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

    const auto it = std::ranges::find_if(m_pins, [pin](const std::unique_ptr<PinInstance>& ptr) noexcept { return ptr.get() == pin; });
    LogRAssert(it != m_pins.end(), "Pin not found. w:" << get_id() << ", p:" << pin->get_instance_id(), );

    Log("Pin removed. w:" << get_id() << ", n:" << pin->get_parent_node() << ", p:" << pin->get_instance_id() << ", t:" << pin->state().type_str);
    m_pins.erase(it);    
}

void Workflow::remove_link(const Link* link)
{
    LogRAssert(link, "Zero ptr", );
    const auto it = std::ranges::find_if(m_links,
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

void Workflow::split_reroutes(const std::set<data::NodeInstance*>& nodes)
{
    m_action->multi_action_on("Split");
    for (auto node : nodes)
    {
        LogCAssert(node->is_cni_reroute(), "Node is not reroute. w:" << get_id() << ", n:" << node->get_instance_id() << ", t:" << typeid(node).name());
        LogCAssert(node->get_inputs().size() == node->get_outputs().size(), "Invalid reroute. w:" << get_id() << ", n:" << node->get_instance_id() << ", sz1:" << node->get_inputs().size() << ", sz2:" << node->get_outputs().size());

        const size_t count = node->get_inputs().size() - 1;
        ImRect rect = node->m_rc;

        for (size_t i = 1; i < count; i++)
        {
            const auto in_pin = node->get_inputs()[i];
            const auto out_pin = node->get_outputs()[i];
            if (in_pin->get_object()->hidden || out_pin->get_object()->hidden)
                continue;

            data::PinInstance* left_pin = nullptr;
            data::PinInstance* right_pin = nullptr;
            data::Link* left_link = nullptr;
            data::Link* right_link = nullptr;

            const auto& links = get_links();
            for (auto& link : links)
            {
                if (link->get_input()->get_instance_id() == in_pin->get_instance_id())
                {
                    left_pin = link->get_output();
                    left_link = link.get();
                }
                else if (link->get_output()->get_instance_id() == out_pin->get_instance_id())
                {
                    right_pin = link->get_input();
                    right_link = link.get();
                }
                if (left_pin != nullptr && right_pin != nullptr)
                    break;
            }

            if (nullptr == left_pin && nullptr == right_pin)
            {
                LogError("Linked pins not found. w:" << get_id() << ", n1:" << in_pin->get_parent_node() << ", p1:" << in_pin->get_instance_id() << ", n2:" << out_pin->get_parent_node() << ", p2:" << out_pin->get_instance_id());
                break;
            }

            const auto* new_node = m_action->add_node(node->get_object(), "", rect);
            LogCAssert(new_node->is_cni_reroute(), "New node is not reroute. w:" << get_id() << ", n:" << new_node->get_instance_id() << ", t:" << typeid(new_node).name());
            LogCAssert(new_node->get_inputs().size() == new_node->get_outputs().size(), "Invalid reroute. w:" << get_id() << ", n:" << new_node->get_instance_id() << ", sz1:" << new_node->get_inputs().size() << ", sz2:" << new_node->get_outputs().size());
            LogCAssert(new_node->get_inputs().size() == 1, "Invalid reroute. w:" << get_id() << ", n:" << new_node->get_instance_id() << ", sz:" << new_node->get_inputs().size());

            if (left_pin != nullptr)
            {
                m_action->unlink(left_link);
                auto new_in_pin = new_node->get_inputs()[0];
                m_action->link(left_pin, new_in_pin);
            }
            if (right_pin != nullptr)
            {
                m_action->unlink(right_link);
                auto new_out_pin = new_node->get_outputs()[0];
                m_action->link(right_pin, new_out_pin);
            }

            rect.Min.x += 10;
            rect.Min.y += 10;
        }
    }
    m_action->multi_action_off();
}

void Workflow::merge_reroutes(const std::set<data::NodeInstance*>& nodes)
{
    if (nodes.size() <= 1)
        return;

    m_action->multi_action_on("Merge");

    data::NodeInstance* dest_node = nullptr;
    for (auto node : nodes)
    {
        LogCAssert(node->is_cni_reroute(), "Node is not reroute. w:" << get_id() << ", n:" << node->get_instance_id() << ", t:" << typeid(node).name());
        LogCAssert(node->get_inputs().size() == node->get_outputs().size(), "Invalid reroute. w:" << get_id() << ", n:" << node->get_instance_id() << ", sz1:" << node->get_inputs().size() << ", sz2:" << node->get_outputs().size());

        if (nullptr == dest_node)
        {
            dest_node = node;
            continue;
        }

        bool node_can_merge = true;
        enum Step { Check, Relink, Finish };

        for (int32_t step = Step::Check; node_can_merge && step < Step::Finish; step++)
        {
            const size_t count = node->get_inputs().size() - 1;
            for (size_t i = 0; node_can_merge && i < count; i++)
            {
                const auto in_pin = node->get_inputs()[i];
                const auto out_pin = node->get_outputs()[i];

                if (in_pin->get_object()->hidden)
                    continue;

                data::PinInstance* left_pin = nullptr;
                data::PinInstance* right_pin = nullptr;
                data::Link* left_link = nullptr;
                data::Link* right_link = nullptr;

                const auto& links = get_links();
                for (auto& link : links)
                {
                    if (link->get_input()->get_instance_id() == in_pin->get_instance_id())
                    {
                        left_pin = link->get_output();
                        left_link = link.get();
                    }
                    else if (link->get_output()->get_instance_id() == out_pin->get_instance_id())
                    {
                        right_pin = link->get_input();
                        right_link = link.get();
                    }
                    if (left_pin != nullptr && right_pin != nullptr)
                        break;
                }

                if (nullptr == left_pin && nullptr == right_pin)
                {
                    node_can_merge = false;
                    LogError("Linked pins not found. w:" << get_id() << ", n1:" << in_pin->get_parent_node() << ", p1:" << in_pin->get_instance_id() << ", n2:" << out_pin->get_parent_node() << ", p2:" << out_pin->get_instance_id());
                    break;
                }

                const size_t dest_idx = dest_node->get_inputs().size() - 1;
                if (left_pin != nullptr)
                {
                    auto new_in_pin = dest_node->get_inputs()[dest_idx];
                    if (Step::Check == step && !Link::can_link(left_pin, new_in_pin))
                    {
                        node_can_merge = false;
                        break;
                    }
                    else if (Step::Relink == step)
                    {
                        m_action->unlink(left_link);
                        m_action->link(left_pin, new_in_pin);
                    }
                }
                if (right_pin != nullptr)
                {
                    auto new_out_pin = dest_node->get_outputs()[dest_idx];
                    if (Step::Check == step && !Link::can_link(right_pin, new_out_pin))
                    {
                        node_can_merge = false;
                        break;
                    }
                    else if (Step::Relink == step)
                    {
                        m_action->unlink(right_link);
                        m_action->link(right_pin, new_out_pin);
                    }
                }
            }
        }

        if (node_can_merge)
            m_action->delete_node(node);
    }
    m_action->multi_action_off();
}

void Workflow::collapse_reroute(const data::NodeInstance* node)
{
    LogRAssert(node, "Zero ptr", );
    LogRAssert(node->is_cni_reroute(), "Node is not reroute. w:" << get_id() << ", n:" << node->get_instance_id() << ", t:" << typeid(node).name(), );
    LogRAssert(node->get_inputs().size() == node->get_outputs().size(), "Invalid reroute. w:" << get_id() << ", n:" << node->get_instance_id() << ", sz1:" << node->get_inputs().size() << ", sz2:" << node->get_outputs().size(), );

    m_action->multi_action_on("Collapse");

    const size_t count = node->get_inputs().size() - 1;

    for (size_t i = 0; i < count; i++)
    {
        const auto in_pin = node->get_inputs()[i];
        const auto out_pin = node->get_outputs()[i];
        if (in_pin->get_object()->hidden || out_pin->get_object()->hidden)
            continue;

        PinInstance* left_pin = nullptr;
        Link* left_link = nullptr;
        std::list<std::pair<PinInstance*, Link*>> right_pin_link;

        const auto& links = get_links();
        for (auto& link : links)
        {
            if (link->get_input()->get_instance_id() == in_pin->get_instance_id())
            {
                left_pin = link->get_output();
                left_link = link.get();
            }
            else if (link->get_output()->get_instance_id() == out_pin->get_instance_id())
            {
                right_pin_link.push_back({ link->get_input(), link.get() });
            }
        }

        if (nullptr == left_pin && right_pin_link.empty())
        {
            LogError("Linked pins not found. w:" << get_id() << ", n1:" << in_pin->get_parent_node() << ", p1:" << in_pin->get_instance_id() << ", n2:" << out_pin->get_parent_node() << ", p2:" << out_pin->get_instance_id());
            break;
        }

        if (left_link != nullptr)
            m_action->unlink(left_link);
        std::ranges::for_each(right_pin_link, [this](const std::pair<PinInstance*, Link*>& pair) { m_action->unlink(pair.second); });

        if (left_pin != nullptr)
        {
            for (const auto& pair : right_pin_link)
            {
                auto right_pin = pair.first;
                if (left_pin->get_parent_node() == right_pin->get_parent_node())
                    continue;

                LogCAssert(Link::can_link(left_pin, right_pin), "Reroute external link can't be linked. w:" << get_id()
                    << ", p1:" << left_pin->get_instance_id() << ":" << left_pin->state().type_str
                    << ", p2:" << right_pin->get_instance_id() << ":" << right_pin->state().type_str);

                m_action->link(left_pin, right_pin);
            }
        }
    }
    
    m_action->multi_action_off();
}

void Workflow::bypass_node(const data::NodeInstance* node)
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
    std::ranges::for_each(nodes_ptr, [&nodes](const std::unique_ptr<NodeInstance>& node) { nodes.push_back(node.get()); });

    for (auto node : nodes)
    {
        bool remove = (node->m_group || node->get_mode() == NodeMode::Mute);

        if (!remove && node->is_cni_reroute())
        {
            collapse_reroute(node);
            remove = true;
        }

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

void Workflow::insert_reroute(data::Link* link, const ImVec2& pos)
{
    LogRAssert(link, "Zero ptr", );

    m_action->multi_action_on("Reroute");

    auto left = link->get_output();
    auto right = link->get_input();

    auto reroute_obj = g::pNodesManager->find_node(base::HashID::make_hash(CNI_CLASS_REROUTE));
    LogRAssert(reroute_obj != nullptr, "Reroute node obj not found", );

    auto* reroute = m_action->add_node(reroute_obj, "", ImRect(pos.x, pos.y, 0, 0));
    LogRAssert(reroute != nullptr, "Reroute node not created", );
    LogRAssert(reroute->is_cni_reroute(), "Node is not reroute. w:" << get_id() << ", n:" << reroute->get_instance_id() << ", t:" << typeid(reroute).name(), );

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedReroute>();
    RoutedPinPair pair;
    pair.type = left->state().type_str;
    ((CNI_DeserializedReroute*)data.get())->pairs.push_back(pair);

    reroute->cni_save_deserialized_info(std::move(data));
    ((CNI_Reroute*)reroute)->cni_init();

    LogRAssert(reroute->get_inputs().size() == reroute->get_outputs().size(), "Invalid reroute. w:" << get_id() << ", n:" << reroute->get_instance_id() << ", sz1:" << reroute->get_inputs().size() << ", sz2:" << reroute->get_outputs().size(), );
    LogRAssert(reroute->get_inputs().size() == 1, "Invalid reroute. w:" << get_id() << ", n:" << reroute->get_instance_id() << ", sz1:" << reroute->get_inputs().size() << ", sz2:" << reroute->get_outputs().size(), );

    auto reroute_in = reroute->get_inputs()[0];
    auto reroute_out = reroute->get_outputs()[0];

    LogRAssert(Link::can_link(left, reroute_in), "Rerouted pins (left) can't be linked. w:" << get_id() << ", l:" << link->get_instance_id()
        << ", p1:" << left->get_instance_id() << ":" << left->state().type_str
        << ", p2:" << reroute_in->get_instance_id() << ":" << reroute_in->state().type_str, );
    LogRAssert(Link::can_link(right, reroute_out), "Rerouted pins (right) can't be linked. w:" << get_id() << ", l:" << link->get_instance_id()
        << ", p1:" << right->get_instance_id() << ":" << right->state().type_str
        << ", p2:" << reroute_out->get_instance_id() << ":" << reroute_out->state().type_str, );

    m_action->unlink(link);
    m_action->link(left, reroute_in);
    m_action->link(right, reroute_out);

    m_action->multi_action_off();
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
        const auto* copy_to_node = (nullptr == copy_to ? nullptr : copy_to->get_node_by_instance(node->get_instance_id()));
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
