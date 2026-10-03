#include "core/workflow/custom_nodes/cni_reroute.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"

namespace data
{

std::unique_ptr<Node> CNI_Reroute::cni_create_object()
{
    auto node = std::make_unique<data::Node>(cni_get_node_class());
    node->name = node->display_name = cni_get_node_class();
    node->category = "utils";
    node->description = "Reroute Node Purpose: Mainly used to organize the logic of overly long connection lines in the " APP_COMFYUI " workflow.";
    node->python_module = "nodes";
    node->python_module_hash = base::HashID::make_hash(node->python_module);
    node->flags.remove(NodeObjFlags::CanBypass, NodeObjFlags::CanRename);
    node->flags.add(NodeObjFlags::SpecReroute, NodeObjFlags::CustomWdgtRestrictCfg);
    node->finalize(nullptr);
    return node;
}

CNI_Reroute::CNI_Reroute(Node* obj, wid workflow_id, iid instance_id, iid instance_id2) noexcept
    : CNI_BaseObj_T<CNI_Reroute>(obj, workflow_id, instance_id)
{
    flags().add(NodeInstFlags::CNI, NodeInstFlags::JsonApiDontExport, NodeInstFlags::JsonCustomSerialization, NodeInstFlags::BlobCustomSerialization);

    static bool r_node_creation = false;
    if (r_node_creation)
        flags().add(NodeInstFlags::RerouteSplitted_R);
    r_node_creation = false;

    auto wf = g::pWfsManager->access(workflow_id);
    if (flags().has(NodeInstFlags::RerouteSplitted_R))
    {
        flags().add(NodeInstFlags::Hidden);
        flags().remove(NodeInstFlags::CanBeDeleted);
        m_data = nullptr; // lets crash, this shouldn't be used
    }
    else
    {
        r_node_creation = true;
        auto node = wf->add_node(obj, {}, {}, instance_id2);
        LogRAssert(node->obj_flags().has(NodeObjFlags::SpecReroute), "Invalid node created. w:" << get_workflow_id() << ", n1:" << get_instance_id() << ", n1:" << node->get_instance_id(), );
        m_splitted_part = cast::strict<CNI_Reroute>(node);
        m_splitted_part->m_splitted_part = this;
        m_data = std::make_shared<RerouteData>();
    }
}

CNI_Reroute::~CNI_Reroute() noexcept
{
    try
    {
        auto wf = g::pWfsManager->access(get_workflow_id());

        if (flags().has(NodeInstFlags::RerouteSplitted_R))
        {
            m_splitted_part->m_splitted_part = nullptr;
            LogAssert(flags().has(NodeInstFlags::CanBeDeleted), "Splitted R part should not be removed prior to L part");
        }
        else
        {
            auto links = wf->get_links_by_pin(m_splitted_pin);
            if (!links.empty())
            {
                m_splitted_pin->obj_flags().add(PinObjFlags::CanUnlink);
                m_splitted_part->m_splitted_pin->obj_flags().add(PinObjFlags::CanUnlink);
                wf->remove_link(links.front());
            }
        }

        cni_remove_pin(m_splitted_pin->get_object());
        wf->remove_pin(m_splitted_pin);
        m_splitted_pin = nullptr;

        if (flags().has_not(NodeInstFlags::RerouteSplitted_R) && m_splitted_part != nullptr)
        {
            m_splitted_part->reset_pins();
            m_splitted_part->flags().add(NodeInstFlags::CanBeDeleted);
            wf->remove_node(m_splitted_part);
        }
    }
    catch (...)
    {
    }
}

void CNI_Reroute::cni_init()
{
    std::unique_ptr<Pin> pin_obj;
    const std::string pin_type = REROUTE_SPLITTED_PIN;

    auto wf = g::pWfsManager->access(get_workflow_id());
    if (flags().has(NodeInstFlags::RerouteSplitted_R))
    {
        cni_recreate_base_object();
        pin_obj = g::pNodesManager->make_pin_ptr(get_object(), pin_type, pin_type, -1);
    }
    else
    {        
        pin_obj = g::pNodesManager->make_pin_ptr(get_object(), pin_type, pin_type, 0);
    }

    pin_obj->flags.remove(PinObjFlags::CanRename, PinObjFlags::CanLink, PinObjFlags::CanUnlink, PinObjFlags::InputCanBeWidget);

    auto instance = wf->add_pin(this, pin_obj.get());
    cni_add_pin(std::move(pin_obj));

    m_splitted_pin = instance;
    m_splitted_pin->state().name.clear();
    m_splitted_pin->state().w_name = 0;
    m_splitted_pin->flags().add(PinInstFlags::RerouteSplittingPin);

    if (flags().has(NodeInstFlags::RerouteSplitted_R))
        return;

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
                m_data->m_next_out_index = std::max(m_data->m_next_out_index, pair.out_index + 1);
            }

            m_data->m_next_out_index = std::max(m_data->m_next_out_index, deserialized_values->next_out_index);
            if (!deserialized_values->in_pin_types.empty())
                m_data->m_in_pin_types = deserialized_values->in_pin_types;

            if (deserialized_values->splitted_node != 0)    m_splitted_part->set_instance_id(deserialized_values->splitted_node);
            if (deserialized_values->splitter_out != 0)     m_splitted_pin->set_instance_id(deserialized_values->splitter_out);
            if (deserialized_values->splitter_in != 0)      m_splitted_part->m_splitted_pin->set_instance_id(deserialized_values->splitter_in);
            if (deserialized_values->splitter_link != 0)    m_splitter_link_id = deserialized_values->splitter_link;
            if (deserialized_values->splitted)
                split_to_parts();
            set_splitted_nodes_poses(deserialized_values->poses);

            m_deserialized_values.reset();
        }
    }
    else
        add_pair();
}

void CNI_Reroute::cni_cleanup_base_object()
{
    if (flags().has(NodeInstFlags::RerouteSplitted_R))
        return;

    cleanup_pairs(false);
}

bool CNI_Reroute::cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    LogRAssert(copy_from_node->obj_flags().has(NodeObjFlags::SpecReroute), "Source node is not reroute", false);
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), false);
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", false);

    const auto* reroute = cast::strict<const CNI_Reroute>(copy_from_node);
    LogRAssert(reroute, "Node is not CNI. w:" << copy_from_node->get_workflow_id() << ", n: " << copy_from_node->get_instance_id() << ", t:" << typeid(copy_from_node).name(), true);
    reroute = cast::strict<const CNI_Reroute>(reroute->get_splitted_base());

    auto& ins = copy_from_node->get_inputs();
    auto& outs = copy_from_node->get_outputs();
    LogRAssert(ins.size() == outs.size(), "Source node pins wrong pair. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", i:" << ins.size() << ", o:" << outs.size(), true);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedReroute>();
    auto reroute_data = cast::strict<CNI_DeserializedReroute>(data.get());
    reroute_data->pairs.reserve(ins.size());

    for (size_t i = 0; i < ins.size(); i++)
    {
        const auto* in = ins[i];
        const auto* out = outs[i];
        if (in->flags().has(PinInstFlags::Hidden) || out->flags().has(PinInstFlags::Hidden))
            continue;

        RoutedPinPair pair;
        pair.in_id = (copy_pin_ids ? in->get_instance_id() : 0);
        pair.out_id = (copy_pin_ids ? out->get_instance_id() : 0);
        pair.type = in->state().type_str;
        pair.in_pin_name = in->get_object()->def_state.name;
        pair.out_index = out->get_object()->out_index;
        reroute_data->pairs.push_back(pair);
    }

    reroute_data->in_pin_types = reroute->m_data->m_in_pin_types;
    reroute_data->next_out_index = reroute->m_data->m_next_out_index;

    if (copy_from_node->flags().has(NodeInstFlags::RerouteSplitted))
    {
        reroute_data->splitted = true;
        reroute_data->poses = reroute->get_splitted_nodes_poses();
        if (copy_pin_ids)
            reroute->get_splitters(reroute_data->splitted_node, reroute_data->splitter_in, reroute_data->splitter_out, reroute_data->splitter_link);
        else
            reroute_data->splitted_node = reroute_data->splitter_in = reroute_data->splitter_out = reroute_data->splitter_link = 0;
    }

    cni_save_deserialized_info(std::move(data));

    return true;
}

void CNI_Reroute::cni_mirror_value_to(const PinInstance* src, std::vector<PinInstance*>& append_pins) const
{
    LogRAssert(src, "Zero ptr", );
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), );
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", );

    for (const auto& pair : m_data->m_pairs)
    {
        if (!pair.linked)
            continue;

        if (pair.input.internal == src)
        {
            std::ranges::for_each(pair.output.external, [&append_pins](const PinInstance* pin) { append_pins.push_back(cast::remove_const<PinInstance>(pin)); });
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
    obj->flags.remove(PinObjFlags::CanRename, PinObjFlags::InputCanBeWidget);
    if (out_index >= 0)
    {
        add_output(instance);
        m_splitted_part->add_output(instance);
    }
    else
    {
        add_input(instance);
        m_splitted_part->add_input(instance);
    }
    cni_add_pin(std::move(obj));
    return instance;
}

void CNI_Reroute::remove_pin_internal(Workflow* wf, PinInstance* pin, bool force_cleanup) noexcept
{
    if (force_cleanup)
    {
        std::erase_if(get_object()->input, [pin = pin->get_object()](const Pin* p) { return pin == p; });
        std::erase_if(get_object()->output, [pin = pin->get_object()](const Pin* p) { return pin == p; });
        cni_remove_pin(pin->get_object());

        remove_pin(pin);
        m_splitted_part->remove_pin(pin);
        wf->remove_pin(pin);
    }
    else if (pin->flags().has_not(PinInstFlags::Hidden))
    {
        pin->flags().add(PinInstFlags::Hidden);
        Log("Pin hidden. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p:" << pin->get_instance_id() << ", t:" << pin->state().type_str);
    }
}

std::string CNI_Reroute::create_pin_name(const std::string& type_str)
{
    const auto type = Pin::type_str_to_type(type_str);
    if (PinType::ANY == type)
        return type_str;

    int32_t real_count = 0;
    for (const auto& pair : m_data->m_pairs)
    {
        if (pair.type == type)
            real_count++;
    }

    m_data->m_in_pin_types[type] = std::max(m_data->m_in_pin_types[type], real_count) + 1;

    return type_str + "_" + std::to_string(m_data->m_in_pin_types[type]);
}

void CNI_Reroute::on_deserialilzation_ends()
{
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", );
    cleanup_pairs(true);
    add_pair({});
}

void CNI_Reroute::get_pins_from_link(const PinInstance* p1, const PinInstance* p2, const PinInstance*& external, const LinkedPair*& pair) const noexcept
{
    external = nullptr;
    pair = nullptr;

    for (auto& paired : m_data->m_pairs)
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

    for (auto& paired : m_data->m_pairs)
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
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), false);
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", false);

    LogRAssert(p1 && p2, "Zero ptr", false);

    const PinInstance* external_pin = nullptr;
    const LinkedPair* pair = nullptr;
    get_pins_from_link(p1, p2, external_pin, pair);
    if (nullptr == external_pin || nullptr == pair) [[unlikely]]
        return false;

    const auto ext_type = external_pin->state().type;
    const auto pair_type = pair->type;

    // prevent linking two completely empty Reroutes (ANY to ANY)
    if (PinType::ANY == ext_type && PinType::ANY == pair_type) [[unlikely]]
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

                obj = g::pNodesManager->make_pin_ptr(get_object(), ext_state.type_str, pair_name, m_data->m_next_out_index++);
                pair.output.internal->set_object(obj.get());
                cni_add_pin(std::move(obj));
            }

            ext_state.copy_to(pair.input.internal->state());
            ext_state.copy_to(pair.output.internal->state());
            pair.input.internal->state().name = pair.output.internal->state().name = pair.input.internal->get_object()->def_state.name;

            auto ext_obj = best_pin->get_object();
            auto int_obj = pair.input.internal->get_object();            

            int_obj->def_state.min = ext_obj->def_state.min;
            int_obj->def_state.max = ext_obj->def_state.max;
            int_obj->def_state.step = ext_obj->def_state.step;
            int_obj->def = ext_obj->def;            
            int_obj->flags.remove(PinObjFlags::CanRename, PinObjFlags::InputCanBeWidget);
            pair.input.internal->reset_state();

            int_obj = pair.output.internal->get_object();
            int_obj->def_state.min = ext_obj->def_state.min;
            int_obj->def_state.max = ext_obj->def_state.max;
            int_obj->def_state.step = ext_obj->def_state.step;
            int_obj->def = ext_obj->def;
            int_obj->flags.remove(PinObjFlags::CanRename, PinObjFlags::InputCanBeWidget);
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
    const int32_t out_index = (pair.out_index < 0 ? m_data->m_next_out_index++ : pair.out_index);

    new_pair.input.internal = add_pin_internal(wf, pair.type, pair_name, -1, pair.in_id);
    new_pair.output.internal = add_pin_internal(wf, pair.type, pair_name, out_index, pair.out_id);

    new_pair.input.internal->flags().add_remove_if(PinInstFlags::Hidden, pair.hidden);
    new_pair.output.internal->flags().add_remove_if(PinInstFlags::Hidden, pair.hidden);

    m_data->m_pairs.push_back(new_pair);
}

void CNI_Reroute::cleanup_pairs(bool keep_linked)
{
    auto wf = g::pWfsManager->access(get_workflow_id());

    for (auto it = m_data->m_pairs.begin(); it != m_data->m_pairs.end(); )
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
                it = m_data->m_pairs.erase(it);
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
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), );

    if (flags().has(NodeInstFlags::RerouteSplitted_R))
        return;
    if (p1->flags().has(PinInstFlags::RerouteSplittingPin) || p2->flags().has(PinInstFlags::RerouteSplittingPin))
    {
        LogRAssert(p1->flags().has(PinInstFlags::RerouteSplittingPin) && p2->flags().has(PinInstFlags::RerouteSplittingPin), "Both pins must be splitting pins", );
        return;
    }

    LogRAssert(p1 && p2, "Zero ptr", );
    LogRAssert(cni_can_link(p1, p2),
        "Check CNI_Reroute can link before entablish connection. w:" << get_workflow_id() << ", n: " << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id(), );

    const PinInstance* external_pin = nullptr;
    LinkedPair* pair = nullptr;
    get_pins_from_link(p1, p2, external_pin, pair);
    LogRAssert(external_pin && pair,
        "Pair not found. w:" << get_workflow_id() << ", n: " << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id(), );

    // register the new connection
    if (external_pin->obj_flags().has(PinObjFlags::Input))
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
        if (pair->input.internal->flags().has(PinInstFlags::Hidden))
            Log("Pin unhidden. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p:" << pair->input.internal->get_instance_id() << ", t:" << pair->input.internal->state().type_str);
        pair->input.internal->flags().remove(PinInstFlags::Hidden);
    }
    if (pair->output.internal != nullptr)
    {
        if (pair->output.internal->flags().has(PinInstFlags::Hidden))
            Log("Pin unhidden. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p:" << pair->output.internal->get_instance_id() << ", t:" << pair->output.internal->state().type_str);
        pair->output.internal->flags().remove(PinInstFlags::Hidden);
    }

    Log("Reroute linked. w:" << get_workflow_id() << ", n:" << get_instance_id() <<
        (p1 == external_pin ? ", ext:" : ", int:") << p1->get_instance_id() <<
        (p1 == external_pin ? ", int:" : ", ext:") << p2->get_instance_id() <<
        ", t:" << external_pin->state().type_str);

    // refresh type and name based on the new connections
    refresh_pair_state(*pair);
    if (flags().has_not(NodeInstFlags::Deserialization))
    {
        cleanup_pairs(true);
        add_pair();
    }
}

void CNI_Reroute::cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2)
{
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), );
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", );

    LogRAssert(p1 && p2, "Zero ptr", );

    const PinInstance* external_pin = nullptr;
    LinkedPair* pair = nullptr;
    get_pins_from_link(p1, p2, external_pin, pair);
    LogRAssert(external_pin && pair,
        "Pair not found. w:" << get_workflow_id() << ", n: " << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id(), );

    // remove the disconnected pin
    if (external_pin->obj_flags().has(PinObjFlags::Input))
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

void CNI_Reroute::split_to_parts()
{
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted), "Reroute already splitted. w:" << get_workflow_id() << ", n:" << get_instance_id(), );
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), );
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", );

    flags().add(NodeInstFlags::RerouteSplitted);
    m_splitted_part->flags().add(NodeInstFlags::RerouteSplitted);
    m_splitted_part->flags().remove(NodeInstFlags::Hidden);

    auto right_pin = m_splitted_part->m_splitted_pin;
    LogRAssert(Link::can_link(m_splitted_pin, right_pin), "Can't link splitted reroutes. w:" << get_workflow_id() << ", n1:" << get_instance_id() << ", n2:" << m_splitted_part->get_instance_id() << ", p1:" << m_splitted_pin->get_instance_id() << ", p2:" << right_pin->get_instance_id(), );
    auto link = Link::link(m_splitted_pin, right_pin);
    link->set_link_thickness_mult(1.5f);
    if (m_splitter_link_id != 0)
        link->set_instance_id(m_splitter_link_id);
    else
        m_splitter_link_id = link->get_instance_id();

    adjust_r_pose();
    Log("Reroute splitted. w:" << get_workflow_id() << ", n:" << get_instance_id());
}

void CNI_Reroute::adjust_r_pose()
{
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), );
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", );

    auto rc = m_rc;
    rc.Min.x = m_rc.Min.x + m_rc.Max.x + 100.0f;
    set_splitted_nodes_poses({ m_rc, rc });
}

void CNI_Reroute::set_splitted_nodes_poses(const std::pair<ImRect, ImRect>& poses) noexcept
{
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), );

    if (flags().has(NodeInstFlags::RerouteSplitted_R))
    {
        m_splitted_part->m_rc = poses.first;
        m_rc = poses.second;
    }
    else
    {
        m_rc = poses.first;
        m_splitted_part->m_rc = poses.second;
    }

    flags().remove(NodeInstFlags::PosSet, NodeInstFlags::SizeSet);
    reset_width();

    m_splitted_part->flags().remove(NodeInstFlags::PosSet, NodeInstFlags::SizeSet);
    m_splitted_part->reset_width();
}

void CNI_Reroute::merge_parts(bool to_left)
{
    LogRAssert(flags().has(NodeInstFlags::RerouteSplitted), "Reroute wasn't splitted. w:" << get_workflow_id() << ", n:" << get_instance_id(), );
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), );
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", );

    auto wf = g::pWfsManager->access(get_workflow_id());
    auto links = wf->get_links_by_pin(m_splitted_pin);

    LogRAssert(links.size() == 1, "Empty splitted links", );
    m_splitted_pin->obj_flags().add(PinObjFlags::CanUnlink);
    m_splitted_part->m_splitted_pin->obj_flags().add(PinObjFlags::CanUnlink);
    m_splitted_part->flags().remove(NodeInstFlags::RerouteSplitted);

    wf->remove_link(links.front());
    m_splitter_link_id = 0;

    flags().remove(NodeInstFlags::RerouteSplitted);
    m_splitted_part->flags().add(NodeInstFlags::Hidden);

    if (!to_left)
        m_rc = m_splitted_part->m_rc;

    reset_width();
    flags().remove(NodeInstFlags::PosSet, NodeInstFlags::SizeSet);
    Log("Reroute merged. w:" << get_workflow_id() << ", n:" << get_instance_id());
}

void CNI_Reroute::get_splitters(iid& splitted_node, iid& in_pin, iid& out_pin, iid& link) const
{
    if (flags().has(NodeInstFlags::RerouteSplitted_R))
        return m_splitted_part->get_splitters(splitted_node, in_pin, out_pin, link);

    splitted_node = in_pin = out_pin = link = 0;

    splitted_node = m_splitted_part->get_instance_id();
    in_pin = m_splitted_part->m_splitted_pin->get_instance_id();
    out_pin = m_splitted_pin->get_instance_id();
    link = m_splitter_link_id;
}

} // namespace data
