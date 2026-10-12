#include "core/workflow/custom_nodes/cni_reroute.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"

namespace data
{

CNI_Reroute::CNI_Reroute(Node* obj, wid workflow_id, iid instance_id, iid instance_id2) noexcept
    : CNI_BaseObj(obj, workflow_id, instance_id)
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
        // create hidden splitters
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

        // base part must be destroyed first - it removes objects, links etc. and then destroys splitters
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
        // dedicated node object for an additional split node
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

    // clean up all pins just in case, although it should be empty
    cleanup_pairs(false);

    // restore deserialized node data or create an empty pair of ANY pins

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
    add_pair_if_empty();
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
        if (in->flags().has(PinInstFlags::Hidden) || out->flags().has(PinInstFlags::Hidden)) // pins are hidden, no need to copy this pair
            continue;

        RoutedPinPair pair;
        pair.in_id = (copy_pin_ids ? in->get_instance_id() : 0);
        pair.in_hash = (copy_pin_ids ? in->get_object()->get_hash() : 0);
        pair.out_id = (copy_pin_ids ? out->get_instance_id() : 0);
        pair.out_hash = (copy_pin_ids ? out->get_object()->get_hash() : 0);
        pair.type = in->state().type_str;
        pair.in_pin_name = in->get_object()->def_state.name;
        pair.out_index = out->get_object()->out_index;
        reroute_data->pairs.push_back(pair);
    }

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

PinInstance* CNI_Reroute::add_pin_internal(Workflow* wf, const std::string& type, const std::string& name, int32_t out_index, iid id, hid hash)
{
    if (nullptr == wf)
        wf = g::pWfsManager->access(get_workflow_id());

    // create new pin_object, add it to node_object
    auto obj = g::pNodesManager->make_pin_ptr(get_object(), type, name, out_index);
    // all reroute pins always have newly generated unique hash to avoid collisions
    obj->force_replace_hash(0 == hash ? base::GlobalID::random() : hash);
    // create a pin instance; it is stored in the workflow as usual
    auto instance = wf->add_pin(this, obj.get(), id);
    obj->flags.remove(PinObjFlags::CanRename, PinObjFlags::InputCanBeWidget);

    // add pin_instance to the node_instance inputs/outputs
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

    // pin object is stored in special cni storage, not in NodesManager,
    // because reroute pins are not unique and dynamically expand on demand
    cni_add_pin(std::move(obj));
    return instance;
}

void CNI_Reroute::remove_pin_internal(Workflow* wf, PinInstance* pin, bool force_cleanup) noexcept
{
    // `force_cleanup` can be set if the pin has never had a link, so we can safely destroy it
    if (force_cleanup)
    {
        // remove the pin object pointer from the node object's inputs/outputs
        std::erase_if(get_object()->input, [pin = pin->get_object()](const Pin* p) { return pin == p; });
        std::erase_if(get_object()->output, [pin = pin->get_object()](const Pin* p) { return pin == p; });

        // remove pin_object from special cni storage (destroy it)
        cni_remove_pin(pin->get_object());

        // remove pin_instance pointer from node_instance inputs/outputs
        remove_pin(pin);
        m_splitted_part->remove_pin(pin);

        // remove pin_instance (destroy it)
        wf->remove_pin(pin);
    }
    // the purpose of hiding a pin: if the pin was ever linked, the history keeps a reference to this connection.
    // during undo/redo, the history will try to restore the removed pin - in this case, we just unhide it.
    // hidden pins must be saved/restored on serialization if we plan to support history restoration
    else if (pin->flags().has_not(PinInstFlags::Hidden))
    {
        pin->flags().add(PinInstFlags::Hidden);
        Log("Pin hidden. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p:" << pin->get_instance_id() << ", t:" << pin->state().type_str);
    }
}

bool CNI_Reroute::pair_is_hidden(const LinkedPair& pair) const noexcept
{
    if (pair.input.internal != nullptr && pair.input.internal->flags().has(PinInstFlags::Hidden))
        return true;
    if (pair.output.internal != nullptr && pair.output.internal->flags().has(PinInstFlags::Hidden))
        return true;
    return false;
}

void CNI_Reroute::refresh_names()
{
    // we need to precalculate the number of pins of each type for cases
    // where one type is unique on the node, while others have multiple connections

    std::unordered_map<PinType, uint32_t> types_count;
    for (const auto& pair : m_data->m_pairs)
    {
        if (!pair_is_hidden(pair))
            types_count[pair.type]++;
    }

    std::unordered_map<PinType, uint32_t> named_count;
    for (auto& pair : m_data->m_pairs)
    {
        if (pair_is_hidden(pair))
            continue;

        std::string name, type;
        if (pair.input.internal != nullptr)
            type = pair.input.internal->get_object()->def_state.type_str;
        else if (pair.output.internal != nullptr)
            type = pair.output.internal->get_object()->def_state.type_str;
        else
            continue;

        named_count[pair.type]++;
        if (1 == types_count[pair.type])
            name = type;
        else
            name = fmt::format(FMT_STRING("{}_{}"), type, named_count[pair.type]);
        
        if (pair.input.internal != nullptr)
            pair.input.internal->state().name = name;
        if (pair.output.internal != nullptr)
            pair.output.internal->state().name = name;
    }

    reset_width();
}

void CNI_Reroute::on_deserialization_ends()
{
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", );
    cleanup_pairs(true); // cleanup unused pairs
    add_pair_if_empty();
}

void CNI_Reroute::get_pins_from_link(const PinInstance* p1, const PinInstance* p2, const PinInstance*& external, const LinkedPair*& pair) const noexcept
{
    external = nullptr;
    pair = nullptr;

    // search existing pairs for external pins by their internal counterparts - const version

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

    // search existing pairs for external pins by their internal counterparts - non-const version

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

void CNI_Reroute::refresh_pair_state(LinkedPair& pair)
{
    // recursion guard:
    // propagate_pair_type will call refresh_pair_state; we must stop here to prevent infinite recursion
    if (m_now_propagading) 
        return;

    // double-dispatch tear guard:
    // if the core graph manager has modified physical links but this node hasn't 
    // received its cni_pin_linked/unlinked callbacks yet, skip evaluation. 
    // the pending callback will re-trigger refresh_pair_state momentarily.
    const bool has_in_link = (pair.input.internal->state().links > 0);
    const bool has_in_ext = (pair.input.external != nullptr);
    if (has_in_link != has_in_ext)
        return;
    const size_t out_links = pair.output.internal->state().links;
    const size_t out_ext_size = pair.output.external.size();
    if (out_links != out_ext_size)
        return;

    m_now_propagading = true;
    // search for the first meaningful pin type

    const PinInstance* best_pin = nullptr;
    std::unordered_set<const PinInstance*> visited_pins;
    auto wf = g::pWfsManager->access(get_workflow_id());

    // traverse the entire island of reroutes to find a real non-reroute data source/sink
    const auto find_concrete_pin = [&](auto& self, const PinInstance* current_pin)
    {
        if (nullptr == current_pin || best_pin != nullptr || visited_pins.contains(current_pin)) 
            return;
        visited_pins.insert(current_pin);

        NodeInstance* ext_node = wf->get_node_by_instance(current_pin->get_parent_node());
        if (nullptr != ext_node && ext_node->obj_flags().has(NodeObjFlags::SpecReroute))
        {
            // it is a reroute, ignore its type and pass through it to find what lies beyond
            auto* reroute = cast::strict<CNI_Reroute>(ext_node);
            reroute = cast::strict<CNI_Reroute>(reroute->get_splitted_base());

            const LinkedPair* attached_pair = nullptr;
            for (const auto& p : reroute->m_data->m_pairs) 
            {
                if (p.input.internal == current_pin || p.output.internal == current_pin) 
                {
                    attached_pair = &p;
                    break;
                }
            }

            if (attached_pair != nullptr) 
            {
                if (attached_pair->input.external != nullptr) 
                {
                    self(self, attached_pair->input.external);
                }
                for (const auto* out_pin : attached_pair->output.external)
                {
                    self(self, out_pin);
                }
            }
        }
        else
        {
            // it is a real node, check if it has a concrete type
            if (PinType::ANY != current_pin->state().type)
            {
                best_pin = current_pin;
            }
        }
    };

    if (pair.input.external != nullptr) 
    {
        find_concrete_pin(find_concrete_pin, pair.input.external);
    }
    if (nullptr == best_pin)
    {
        for (const auto* out_pin : pair.output.external)
        {
            find_concrete_pin(find_concrete_pin, out_pin);
        }
    }

    if (best_pin != nullptr) 
    {
        auto& ext_state = best_pin->state();

        // update pair type if the external pin has a concrete type
        // remove old pin objects, then re-create and attach a pair of a known type, keep pin hashes and out index

        const auto in_hash = pair.input.internal->get_object()->get_hash();
        const auto out_hash = pair.output.internal->get_object()->get_hash();
        const auto out_index = pair.output.internal->get_object()->out_index;
        cni_remove_pin(pair.input.internal->get_object());
        cni_remove_pin(pair.output.internal->get_object());

        auto obj = g::pNodesManager->make_pin_ptr(get_object(), ext_state.type_str, ext_state.type_str, -1);
        obj->force_replace_hash(in_hash);
        pair.input.internal->set_object(obj.get());
        cni_add_pin(std::move(obj));

        obj = g::pNodesManager->make_pin_ptr(get_object(), ext_state.type_str, ext_state.type_str, out_index);
        obj->force_replace_hash(out_hash);
        pair.output.internal->set_object(obj.get());
        cni_add_pin(std::move(obj));

        // copy pin types and all required data like min, max, step, etc. - we will need it to pass further

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
    else if (nullptr == pair.input.external && pair.output.external.empty())
    {
        // reset pair to ANY state
        // remove old pin objects, then re-create and attach a pair of a ANY type, keep pin hashes and out index

        const auto in_hash = pair.input.internal->get_object()->get_hash();
        const auto out_hash = pair.output.internal->get_object()->get_hash();
        const auto out_index = pair.output.internal->get_object()->out_index;
        cni_remove_pin(pair.input.internal->get_object());
        cni_remove_pin(pair.output.internal->get_object());

        auto obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "*", -1);
        obj->force_replace_hash(in_hash);
        pair.input.internal->set_object(obj.get());
        obj->def_state.copy_to(pair.input.internal->state());
        cni_add_pin(std::move(obj));

        obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "*", out_index);
        obj->force_replace_hash(out_hash);
        pair.output.internal->set_object(obj.get());
        obj->def_state.copy_to(pair.output.internal->state());
        cni_add_pin(std::move(obj));

        pair.type = PinType::ANY;
    }

    reset_width();
    refresh_names();

    propagate_pair_type(pair);
    m_now_propagading = false;
}

void CNI_Reroute::propagate_pair_type(const LinkedPair& pair)
{
    const auto find_pair = [](RerouteData& all_pairs, const data::PinInstance* pin) -> LinkedPair*
    {
        for (auto& pair : all_pairs.m_pairs)
        {
            if (pair.input.internal == pin || pair.output.internal == pin)
                return &pair;
        }
        return nullptr;
    };

    auto wf = g::pWfsManager->access(get_workflow_id());
    std::list<const PinInstance*> ext_pins;
    ext_pins.push_back(pair.input.external);
    ext_pins.insert(ext_pins.end(), pair.output.external.begin(), pair.output.external.end());

    for (const auto* ext_pin : ext_pins)
    {
        if (nullptr == ext_pin)
            continue;
        auto node = wf->get_node_by_instance(ext_pin->get_parent_node());
        if (node->obj_flags().has(NodeObjFlags::SpecReroute))
        {
            auto ext_reroute = cast::strict<CNI_Reroute>(cast::strict<CNI_Reroute>(node)->get_splitted_base());
            auto ext_pair = find_pair(*ext_reroute->m_data, ext_pin);
            if (nullptr == ext_pair)
                LogAssert(ext_pair, "External pair not found");
            else
                ext_reroute->refresh_pair_state(*ext_pair);
        }
    }
}

void CNI_Reroute::add_pair_if_empty()
{
    bool node_empty = true;
    for (const auto& p : m_data->m_pairs)
    {
        if (!pair_is_hidden(p))
        {
            node_empty = false;
            break;
        }
    }
    if (node_empty)
        add_pair();
}

void CNI_Reroute::add_pair(const RoutedPinPair& pair/* = {}*/)
{
    // this function can be called from the UI with a default parameter - pair will default to ANY.
    // if pair.type or pair.in_pin_name is empty, someone called the function and cleared all fields for some reason - this is an error.

    LogRAssert(!pair.type.empty() && !pair.in_pin_name.empty(), "Invalid pair", );

    auto wf = g::pWfsManager->access(get_workflow_id());
    LinkedPair new_pair;

    new_pair.type = Pin::type_str_to_type(pair.type);
    const int32_t out_index = (pair.out_index < 0 ? m_data->m_next_out_index++ : pair.out_index);

    new_pair.input.internal = add_pin_internal(wf, pair.type, pair.type, -1, pair.in_id, pair.in_hash);
    new_pair.output.internal = add_pin_internal(wf, pair.type, pair.type, out_index, pair.out_id, pair.out_hash);

    new_pair.input.internal->flags().add_remove_if(PinInstFlags::Hidden, pair.hidden);
    new_pair.output.internal->flags().add_remove_if(PinInstFlags::Hidden, pair.hidden);

    m_data->m_pairs.push_back(new_pair);
    refresh_names();
}

std::pair<PinInstance*, PinInstance*> CNI_Reroute::find_first_empty_pair_or_add_empty()
{
    std::pair<PinInstance*, PinInstance*> ret;
    for (auto& pair : m_data->m_pairs)
    {
        if (pair_is_hidden(pair))
            continue;
        if (pair.ever_linked || pair.linked)
            continue;
        LogCAssert(PinType::ANY == pair.type, "Empty pair is not ANY");
        ret.first = pair.input.internal;
        ret.second = pair.output.internal;
        return ret;
    }

    add_pair();
    auto& pair = m_data->m_pairs.back();
    ret.first = pair.input.internal;
    ret.second = pair.output.internal;
    return ret;
}

void CNI_Reroute::cleanup_pairs(bool keep_linked)
{
    auto wf = g::pWfsManager->access(get_workflow_id());

    // if the reroute was created as a copy, it might have empty pins.
    // we don't need them because the history knows nothing about them,
    // and we don't want to keep empty pairs on the node.
    // `keep_linked=false` means we want to completely erase all data on the node

    for (auto it = m_data->m_pairs.begin(); it != m_data->m_pairs.end(); )
    {
        auto& pair = *it;
        if (!keep_linked || keep_linked && !pair.linked)
        {
            const bool erase_pins = !keep_linked || !pair.ever_linked && (PinType::_none == pair.type || PinType::ANY == pair.type);
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
            ++it;
    }

    refresh_names();
}

bool CNI_Reroute::cni_can_link(const PinInstance* p1, const PinInstance* p2) const noexcept
{
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), false);
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", false);
    LogRAssert(p1 && p2, "Zero ptr", false);

    if (p1->flags().has(PinInstFlags::RerouteSplittingPin) || p2->flags().has(PinInstFlags::RerouteSplittingPin))
    {
        // came here from split_to_parts
        // check if the logic is correct (both pins must have the same type), then exit
        LogRAssert(p1->flags().has(PinInstFlags::RerouteSplittingPin) && p2->flags().has(PinInstFlags::RerouteSplittingPin), "Both pins must be RerouteSplittingPin", false);
        return true;
    }

    const PinInstance* external_pin = nullptr;
    const LinkedPair* pair = nullptr;
    get_pins_from_link(p1, p2, external_pin, pair);
    if (nullptr == external_pin || nullptr == pair) [[unlikely]]
        return false;

    const auto ext_type = external_pin->state().type;
    const auto pair_type = pair->type;

    // if both pins are ANY (reroute to reroute, reroute to any other ANY) - allow the link
    // if one is ANY and the other has a concrete type, allow the link
    // special case PrimitiveNode<->Reroute is handled and blocked in CNI_Primitive::cni_can_link    
    if (PinType::ANY == ext_type || PinType::ANY == pair_type)
        return true;

    // if both have concrete types, they must strictly match
    return ext_type == pair_type;
}

void CNI_Reroute::cni_pin_linked(PinInstance* p1, PinInstance* p2)
{
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), );

    if (flags().has(NodeInstFlags::RerouteSplitted_R)) // linkage should be handled only once and only on the base node
        return;
    LogRAssert(p1 && p2, "Zero ptr", );
    LogRAssert(cni_can_link(p1, p2), "Check CNI_Reroute::can_link before entablish connection. w:" << get_workflow_id() << ", n: " << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id(), );

    if (p1->flags().has(PinInstFlags::RerouteSplittingPin) || p2->flags().has(PinInstFlags::RerouteSplittingPin))
    {
        // came here from split_to_parts
        // check if the logic is correct (both pins must have the same type), then exit
        LogAssert(p1->flags().has(PinInstFlags::RerouteSplittingPin) && p2->flags().has(PinInstFlags::RerouteSplittingPin), "Both pins must be RerouteSplittingPin");
        return;
    }

    const PinInstance* external_pin = nullptr;
    LinkedPair* pair = nullptr;
    get_pins_from_link(p1, p2, external_pin, pair);
    LogRAssert(external_pin && pair, "Pair not found. w:" << get_workflow_id() << ", n: " << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id(), );

    // register the new connection
    if (external_pin->obj_flags().has(PinObjFlags::Input))
        pair->output.external.push_back(external_pin);
    else
        pair->input.external = external_pin;

    pair->linked = true;
    pair->ever_linked = true;

    // if the linking command came from history, restore the previously hidden pin
    if (pair->input.internal != nullptr && pair->input.internal->flags().has(PinInstFlags::Hidden))
    {
        Log("Pin unhidden. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p:" << pair->input.internal->get_instance_id() << ", t:" << pair->input.internal->state().type_str);
        pair->input.internal->flags().remove(PinInstFlags::Hidden);
    }
    if (pair->output.internal != nullptr && pair->output.internal->flags().has(PinInstFlags::Hidden))
    {
        Log("Pin unhidden. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p:" << pair->output.internal->get_instance_id() << ", t:" << pair->output.internal->state().type_str);
        pair->output.internal->flags().remove(PinInstFlags::Hidden);
    }

    Log("Reroute linked. w:" << get_workflow_id() << ", n:" << get_instance_id() <<
        (p1 == external_pin ? ", ext:" : ", int:") << p1->get_instance_id() <<
        (p1 == external_pin ? ", int:" : ", ext:") << p2->get_instance_id() <<
        ", t:" << external_pin->state().type_str);

    // refresh type and name based on the new connections
    refresh_pair_state(*pair);
}

void CNI_Reroute::cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2)
{
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), );
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", );
    LogRAssert(p1 && p2, "Zero ptr", );

    if (p1->flags().has(PinInstFlags::RerouteSplittingPin) || p2->flags().has(PinInstFlags::RerouteSplittingPin))
    {
        // came here from split_to_parts
        // check if the logic is correct (both pins must have the same type), then exit
        LogAssert(p1->flags().has(PinInstFlags::RerouteSplittingPin) && p2->flags().has(PinInstFlags::RerouteSplittingPin), "Both pins must be RerouteSplittingPin");
        return;
    }

    const PinInstance* external_pin = nullptr;
    LinkedPair* pair = nullptr;
    get_pins_from_link(p1, p2, external_pin, pair);
    LogRAssert(external_pin && pair, "Pair not found. w:" << get_workflow_id() << ", n: " << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id(), );

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
        refresh_pair_state(*pair);
    }

    reset_width();
    refresh_names();
    Log("Reroute unlinked. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id() << ", t:" << p1->state().type_str);    
}

void CNI_Reroute::split_to_parts()
{
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted), "Reroute already splitted. w:" << get_workflow_id() << ", n:" << get_instance_id(), );
    LogRAssert(m_splitted_part, "Reroute splitted part Zero ptr. w:" << get_workflow_id() << ", n:" << get_instance_id(), );
    LogRAssert(flags().has_not(NodeInstFlags::RerouteSplitted_R), "Invalid logic. Splitted R part shouldn't be here", );

    // mark nodes as split, unhide the second part, and link the two splitter pins

    flags().add(NodeInstFlags::RerouteSplitted);
    m_splitted_part->flags().add(NodeInstFlags::RerouteSplitted);
    m_splitted_part->flags().remove(NodeInstFlags::Hidden);

    auto right_pin = m_splitted_part->m_splitted_pin;
    LogRAssert(Link::can_link(m_splitted_pin, right_pin), "Can't link splitted reroutes. w:" << get_workflow_id() << ", n1:" << get_instance_id() << ", n2:" << m_splitted_part->get_instance_id() << ", p1:" << m_splitted_pin->get_instance_id() << ", p2:" << right_pin->get_instance_id(), );
    auto link = Link::link(m_splitted_pin, right_pin);

    link->set_link_thickness_mult(1.5f);
    if (m_splitter_link_id != 0)    // this can happen if we came from deserialization
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

    // remove the link, hide the second node, and remove the splitted mark from the node

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
