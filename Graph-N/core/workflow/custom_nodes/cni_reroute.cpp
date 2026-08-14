#include "core/workflow/custom_nodes/cni_reroute.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"

namespace data
{

std::unique_ptr<Node> CNI_Reroute::cni_create_object(mngr::NodesManager* manager)
{
    if (nullptr == manager)
        manager = g::pNodesManager;

    auto node = std::make_unique<data::Node>(cni_get_node_class());
    node->name = node->display_name = cni_get_node_class();
    node->category = "utils";
    node->description = "Reroute Node Purpose: Mainly used to organize the logic of overly long connection lines in the " APP_COMFYUI " workflow.";
    node->can_bypass = false;
    node->can_rename = false;

    node->finalize(nullptr);
    return node;
}

void CNI_Reroute::cni_cleanup_base_object()
{
    cleanup_pairs(false);
}

bool CNI_Reroute::cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    auto reroute = cast::dynamic<const CNI_Reroute>(copy_from_node);
    LogRAssert(reroute, "Node is not CNI. w:" << copy_from_node->get_workflow_id() << ", n: " << copy_from_node->get_instance_id() << ", t:" << typeid(copy_from_node).name(), true);

    auto& ins = copy_from_node->get_inputs();
    auto& outs = copy_from_node->get_outputs();
    LogRAssert(ins.size() == outs.size(), "Source node pins wrong pair. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", i:" << ins.size() << ", o:" << outs.size(), true);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedReroute>();
    ((CNI_DeserializedReroute*)data.get())->pairs.reserve(ins.size());

    for (size_t i = 0; i < ins.size(); i++)
    {
        const auto* in = ins[i];
        const auto* out = outs[i];
        if (in->get_object()->hidden || out->get_object()->hidden)
            continue;

        RoutedPinPair pair;
        pair.in_id = (copy_pin_ids ? in->get_instance_id() : 0);
        pair.out_id = (copy_pin_ids ? out->get_instance_id() : 0);
        pair.type = in->state().type_str;
        pair.in_pin_name = in->get_object()->def_state.name;
        pair.out_index = out->get_object()->out_index;
        ((CNI_DeserializedReroute*)data.get())->pairs.push_back(pair);
    }

    ((CNI_DeserializedReroute*)data.get())->in_pin_types = reroute->m_in_pin_types;
    ((CNI_DeserializedReroute*)data.get())->next_out_index = reroute->m_next_out_index;

    cni_save_deserialized_info(std::move(data));

    return true;
}

void CNI_Reroute::cni_mirror_value_to(const PinInstance* src, std::vector<PinInstance*>& append_pins) const
{
    LogRAssert(src, "Zero ptr", );

    for (const auto& pair : m_pairs)
    {
        if (!pair.linked)
            continue;

        if (pair.input.internal == src)
        {
            std::ranges::for_each(pair.output.external, [&append_pins](const PinInstance* pin) { append_pins.push_back(cast::remove_const(pin)); });
            break;
        }
    }
}

PinInstance* CNI_Reroute::add_pin_internal(Workflow* wf, const std::string& type, const std::string& name, int32_t out_index, iid id)
{
    if (nullptr == wf)
        wf = g::pWfsManager->access(get_workflow_id());

    auto obj = g::pNodesManager->make_pin_ptr(get_object(), type, name, out_index);
    auto instance = wf->add_pin(this, obj.get(), id);
    obj->can_rename = false;
    obj->input_can_be_widget = false;
    if (out_index >= 0)
        add_output(instance);
    else
        add_input(instance);
    cni_add_pin(std::move(obj));
    return instance;
}

void CNI_Reroute::remove_pin_internal(Workflow* wf, PinInstance* pin, bool force_cleanup) noexcept
{
    m_removed.insert(pin->reset_state());

    if (force_cleanup)
    {
        cni_remove_pin(pin->get_object());
        remove_pin(pin);
        wf->remove_pin(pin);
    }
    else if (!pin->get_object()->hidden)
    {
        pin->get_object()->hidden = true;
        Log("Pin hidden. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p:" << pin->get_instance_id() << ", t:" << pin->state().type_str);
    }
}

std::string CNI_Reroute::create_pin_name(const std::string& type_str)
{
    const auto type = Pin::type_str_to_type(type_str);
    if (PinType::ANY == type)
        return type_str;

    int32_t real_count = 0;
    for (const auto& pair : m_pairs)
    {
        if (pair.type == type)
            real_count++;
    }

    m_in_pin_types[type] = std::max(m_in_pin_types[type], real_count) + 1;

    return type_str + "_" + std::to_string(m_in_pin_types[type]);
}

void CNI_Reroute::cni_init()
{
    cleanup_pairs(false);

    CNI_DeserializedReroute* deserialized_values = nullptr;
    if (m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedReroute>(m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values. w:" << get_workflow_id() << ", n:" << get_instance_id() << " t:" << typeid(m_deserialized_values.get()).name() << ")");

        if (deserialized_values != nullptr)
        {
            for (const auto& pair : deserialized_values->pairs)
            {
                add_pair(pair);
                m_next_out_index = std::max(m_next_out_index, pair.out_index + 1);
            }

            m_next_out_index = std::max(m_next_out_index, deserialized_values->next_out_index);
            if (!deserialized_values->in_pin_types.empty())
                m_in_pin_types = deserialized_values->in_pin_types;

            m_deserialized_values.reset();
        }
    }
    else
        add_pair();
}

void CNI_Reroute::cni_deserialilzation_begins() noexcept
{
    m_deserialization_mode = true;
}

void CNI_Reroute::cni_deserialilzation_ends()
{
    m_deserialization_mode = false;
    cleanup_pairs(true);
    add_pair({});
}

void CNI_Reroute::get_pins_from_link(const PinInstance* p1, const PinInstance* p2, const PinInstance*& external, const LinkedPair*& pair) const noexcept
{
    external = nullptr;
    pair = nullptr;

    for (auto& paired : m_pairs)
    {
        if (paired.input.internal == p1 || paired.output.internal == p1)
        {
            external = p2;
            pair = &paired;
            break;
        }
        if (paired.input.internal == p2 || paired.output.internal == p2)
        {
            external = p1;
            pair = &paired;
            break;
        }
    }
}

void CNI_Reroute::get_pins_from_link(const PinInstance* p1, const PinInstance* p2, const PinInstance*& external, LinkedPair*& pair) noexcept
{
    external = nullptr;
    pair = nullptr;

    for (auto& paired : m_pairs)
    {
        if (paired.input.internal == p1 || paired.output.internal == p1)
        {
            external = p2;
            pair = &paired;
            break;
        }
        if (paired.input.internal == p2 || paired.output.internal == p2)
        {
            external = p1;
            pair = &paired;
            break;
        }
    }
}

bool CNI_Reroute::cni_can_link(const PinInstance* p1, const PinInstance* p2) const noexcept
{
    LogRAssert(p1 && p2, "Zero ptr", false);

    const PinInstance* external_pin = nullptr;
    const LinkedPair* pair = nullptr;
    get_pins_from_link(p1, p2, external_pin, pair);
    if (nullptr == external_pin || nullptr == pair)
        return false;

    const auto ext_type = external_pin->state().type;
    const auto pair_type = pair->type;

    // prevent linking two completely empty Reroutes (ANY to ANY)
    if (PinType::ANY == ext_type && PinType::ANY == pair_type)
        return false;

    // if one is ANY and the other has a concrete type, allow the link
    if (PinType::ANY == ext_type || PinType::ANY == pair_type)
        return true;

    // if both have concrete types, they must strictly match
    return ext_type == pair_type;
}

void CNI_Reroute::refresh_pair_state(LinkedPair& pair)
{
    const PinInstance* best_pin = nullptr;

    if (pair.input.external != nullptr) // prioritize input connection (the source of data drives the name/type)
        best_pin = pair.input.external;
    else if (!pair.output.external.empty()) // fallback to the first available output connection
        best_pin = pair.output.external.front();

    if (best_pin != nullptr)
    {
        auto& ext_state = best_pin->state();

        // prevent degrading to ANY if we already hold a concrete type
        if (ext_state.type != PinType::ANY || PinType::ANY == pair.type)
        {
            if (PinType::ANY == pair.input.internal->get_object()->def_state.type)
            {
                const std::string pair_name = create_pin_name(ext_state.type_str);

                cni_remove_pin(pair.input.internal->get_object());
                cni_remove_pin(pair.output.internal->get_object());

                auto obj = g::pNodesManager->make_pin_ptr(get_object(), ext_state.type_str, pair_name, -1);
                pair.input.internal->set_object(obj.get());
                cni_add_pin(std::move(obj));

                obj = g::pNodesManager->make_pin_ptr(get_object(), ext_state.type_str, pair_name, m_next_out_index++);
                pair.output.internal->set_object(obj.get());
                cni_add_pin(std::move(obj));
            }

            ext_state.copy_to(pair.input.internal->state());
            ext_state.copy_to(pair.output.internal->state());
            pair.input.internal->state().name = pair.output.internal->state().name = pair.input.internal->get_object()->def_state.name;

            auto ext_obj = best_pin->get_object();
            auto int_obj = pair.input.internal->get_object();            

            int_obj->min = ext_obj->min;
            int_obj->max = ext_obj->max;
            int_obj->def = ext_obj->def;            
            int_obj->can_rename = false;
            int_obj->input_can_be_widget = false;
            pair.input.internal->reset_state();

            int_obj = pair.output.internal->get_object();
            int_obj->min = ext_obj->min;
            int_obj->max = ext_obj->max;
            int_obj->def = ext_obj->def;
            int_obj->can_rename = false;
            int_obj->input_can_be_widget = false;
            pair.output.internal->reset_state();

            pair.type = ext_state.type;
        }

        reset_width();
    }
}

void CNI_Reroute::add_pair(const RoutedPinPair& pair/* = {}*/)
{
    LogRAssert(!pair.type.empty(), "Empty pair type", );

    auto wf = g::pWfsManager->access(get_workflow_id());
    LinkedPair new_pair;

    new_pair.type = Pin::type_str_to_type(pair.type);
    const std::string pair_name = (pair.in_pin_name.empty() || pair.in_pin_name == "*") ? create_pin_name(pair.type) : pair.in_pin_name;
    const int32_t out_index = (pair.out_index < 0 ? m_next_out_index++ : pair.out_index);

    new_pair.input.internal = add_pin_internal(wf, pair.type, pair_name, -1, pair.in_id);
    new_pair.output.internal = add_pin_internal(wf, pair.type, pair_name, out_index, pair.out_id);

    new_pair.input.internal->get_object()->hidden = pair.hidden;
    new_pair.output.internal->get_object()->hidden = pair.hidden;

    m_pairs.push_back(new_pair);
}

void CNI_Reroute::cleanup_pairs(bool keep_linked)
{
    auto wf = g::pWfsManager->access(get_workflow_id());

    for (auto it = m_pairs.begin(); it != m_pairs.end(); )
    {
        auto& pair = *it;
        if (!keep_linked || keep_linked && !pair.linked)
        {
            const bool erase_pins = !pair.ever_linked && (PinType::_none == pair.type || PinType::ANY == pair.type);
            if (pair.input.internal != nullptr)
            {
                remove_pin_internal(wf, pair.input.internal, erase_pins);
            }
            if (pair.output.internal != nullptr)
            {
                remove_pin_internal(wf, pair.output.internal, erase_pins);
            }

            if (erase_pins)
                it = m_pairs.erase(it);
            else
                ++it;
        }
        else
        {
            ++it;
        }
    }
}

void CNI_Reroute::cni_pin_linked(PinInstance* p1, PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );
    LogRAssert(cni_can_link(p1, p2),
        "Check CNI_primitive can link before entablish connection. w:" << get_workflow_id() << ", n: " << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id(), );

    const PinInstance* external_pin = nullptr;
    LinkedPair* pair = nullptr;
    get_pins_from_link(p1, p2, external_pin, pair);
    LogRAssert(external_pin && pair,
        "Pair not found. w:" << get_workflow_id() << ", n: " << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id(), );

    // register the new connection
    if (external_pin->get_object()->input)
    {
        pair->output.external.push_back(external_pin);
    }
    else
    {
        pair->input.external = external_pin;
    }

    pair->linked = true;
    pair->ever_linked = true;
    if (pair->input.internal != nullptr)
    {
        if (pair->input.internal->get_object()->hidden)
            Log("Pin unhidden. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p:" << pair->input.internal->get_instance_id() << ", t:" << pair->input.internal->state().type_str);
        pair->input.internal->get_object()->hidden = false;
    }
    if (pair->output.internal != nullptr)
    {
        if (pair->output.internal->get_object()->hidden)
            Log("Pin unhidden. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p:" << pair->output.internal->get_instance_id() << ", t:" << pair->output.internal->state().type_str);
        pair->output.internal->get_object()->hidden = false;        
    }

    Log("Reroute linked. w:" << get_workflow_id() << ", n:" << get_instance_id() <<
        (p1 == external_pin ? ", ext:" : ", int:") << p1->get_instance_id() <<
        (p1 == external_pin ? ", int:" : ", ext:") << p2->get_instance_id() <<
        ", t:" << external_pin->state().type_str);

    // refresh type and name based on the new connections
    refresh_pair_state(*pair);
    if (!m_deserialization_mode)
    {
        cleanup_pairs(true);
        add_pair();
    }
}

void CNI_Reroute::cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );

    const PinInstance* external_pin = nullptr;
    LinkedPair* pair = nullptr;
    get_pins_from_link(p1, p2, external_pin, pair);
    LogRAssert(external_pin && pair,
        "Pair not found. w:" << get_workflow_id() << ", n: " << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id(), );

    // remove the disconnected pin
    if (external_pin->get_object()->input)
    {
        for (auto it = pair->output.external.begin(); it != pair->output.external.end(); ++it)
        {
            if (*it == external_pin)
            {
                pair->output.external.erase(it);
                break;
            }
        }
    }
    else
    {
        pair->input.external = nullptr;
    }

    // if completely isolated, mark as unlinked for deletion
    if (nullptr == pair->input.external && pair->output.external.empty())
    {
        pair->linked = false;        
    }
    else
    {
        // otherwise, refresh type and name from remaining connections
        refresh_pair_state(*pair);
    }

    Log("Reroute unlinked. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id() << ", t:" << p1->state().type_str);

    cleanup_pairs(true);
    add_pair();
    reset_width();
}

void CNI_Reroute::reroute_split_to_parts()
{
    LogRAssert(!is_cni_reroute_splitted(), "Reroute already splitted. w:" << get_workflow_id() << ", n:" << get_instance_id(), );

    m_splitted = true;
    m_ed_splitted.reset();
    m_splitted_pos = m_rc.Min;
    m_splitted_pos.x += m_rc.Max.x + 100.0f;
    m_splitted_pos_set = false;

    const std::string pin_type = REROUTE_SPLITTED_PIN;

    auto wf = g::pWfsManager->access(get_workflow_id());
    auto obj = g::pNodesManager->make_pin_ptr(get_object(), pin_type, pin_type + "_in", -1);
    obj->can_rename = false;
    obj->can_link = false;
    obj->can_unlink = false;
    obj->input_can_be_widget = false;
    auto instance = wf->add_pin(this, obj.get());
    cni_add_pin(std::move(obj));
    m_splitted_in = instance;
    m_splitted_in->state().name.clear();
    m_splitted_in->state().w_name = 0;

    obj = g::pNodesManager->make_pin_ptr(get_object(), pin_type, pin_type + "_out", 0);
    obj->can_rename = false;
    obj->can_link = false;
    obj->can_unlink = false;
    obj->input_can_be_widget = false;
    instance = wf->add_pin(this, obj.get());
    cni_add_pin(std::move(obj));
    m_splitted_out = instance;
    m_splitted_out->state().name.clear();
    m_splitted_out->state().w_name = 0;

    m_splitted_in->set_is_reroute_splitting_pin();
    m_splitted_out->set_is_reroute_splitting_pin();

    LogRAssert(Link::can_link(m_splitted_in, m_splitted_out), "Can't link splitted reroutes. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", pi:" << m_splitted_in->get_instance_id() << ", po:" << m_splitted_out->get_instance_id(), );
    auto link = Link::link(m_splitted_in, m_splitted_out);
    link->set_link_thickness(link->get_link_thickness() * 2);

    reset_width();
    m_pos_set = false;
    m_size_set = false;
    m_drawing_splitted_part = DrawingPart::Left;
}

void CNI_Reroute::reroute_merge_parts(bool to_left)
{
    LogRAssert(is_cni_reroute_splitted(), "Reroute wasn't splitted. w:" << get_workflow_id() << ", n:" << get_instance_id(), );

    m_splitted = false;
    m_ed_splitted.reset();

    auto wf = g::pWfsManager->access(get_workflow_id());
    auto links = wf->get_links_by_pin(m_splitted_in);

    LogRAssert(links.size() == 1, "Empty splitted links", );
    LogRAssert(m_splitted_in, "Zero splitted in pin", );
    LogRAssert(m_splitted_in->get_object(), "Zero splitted in pin obj", );
    LogRAssert(m_splitted_out, "Zero splitted in pin", );
    LogRAssert(m_splitted_out->get_object(), "Zero splitted in pin obj", );

    m_splitted_in->get_object()->can_unlink = true;
    m_splitted_out->get_object()->can_unlink = true;

    wf->remove_link(links.front());

    cni_remove_pin(m_splitted_in->get_object());
    wf->remove_pin(m_splitted_in);
    m_splitted_in = nullptr;

    cni_remove_pin(m_splitted_out->get_object());
    wf->remove_pin(m_splitted_out);
    m_splitted_out = nullptr;

    reset_width();
    m_pos_set = false;
    m_size_set = false;

    if (!to_left)
        m_rc.Min = m_splitted_pos;
}

uint64_t CNI_Reroute::reset_state() noexcept
{
    auto ret = CNI_BaseObj_T<CNI_Reroute>::reset_state();
    m_ed_splitted.reset();
    return ret;
}

std::unordered_set<uint64_t> CNI_Reroute::cni_get_all_eds()
{
    std::unordered_set<uint64_t> eds = m_removed;

    eds.insert(m_ed_splitted.id());
    m_ed_splitted.reset();

    for (auto& pair : m_pairs)
    {
        if (pair.input.internal != nullptr)
            eds.insert(pair.input.internal->reset_state());
        if (pair.output.internal != nullptr)
            eds.insert(pair.output.internal->reset_state());
    }
    
    if (m_splitted_in != nullptr)
        eds.insert(m_splitted_in->reset_state());
    if (m_splitted_out != nullptr)
        eds.insert(m_splitted_out->reset_state());

    return eds;
}

} // namespace data
