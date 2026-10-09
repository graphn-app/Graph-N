#include "core/workflow/link.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/node_instance.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/settings_manager.h"

namespace data
{

Link::Link(wid workflow_id, iid link_id) noexcept
    : m_workflow_id(workflow_id)
    , m_iid(link_id)
{
}

Link::~Link() noexcept = default;

bool Link::can_link(const PinInstance* i1, const PinInstance* i2)
{
    LogRAssertS(i1 && i2, "Zero ptr", false);
    LogRAssertS(i1->get_workflow_id() && i2->get_workflow_id(),
        "Different workflows. w1:" << i1->get_workflow_id() << ", p1:" << i1->get_instance_id() << ", w2:" << i2->get_workflow_id() << ", p2:" << i2->get_instance_id(),
        false);

    const auto p1 = i1->get_object();
    const auto p2 = i2->get_object();
    LogRAssertS(p1 && p2, "Zero ptr", false);

    if (i1->flags().has(PinInstFlags::RerouteSplittingPin) && i2->flags().has(PinInstFlags::RerouteSplittingPin))
        return true;
    if (i1->flags().has(PinInstFlags::RerouteSplittingPin) || i2->flags().has(PinInstFlags::RerouteSplittingPin))
        return false;

    if (p1->flags.has_not(PinObjFlags::CanLink) || p2->flags.has_not(PinObjFlags::CanLink))
        return false;
    if (i1 == i2 || i1->get_parent_node() == i2->get_parent_node())     // same pin, same node
        return false;

    const auto& s1 = i1->state();
    const auto& s2 = i2->state();

    if (p1->flags.has(PinObjFlags::Input) == p2->flags.has(PinObjFlags::Input) ||
        p1->flags.has(PinObjFlags::Output) == p2->flags.has(PinObjFlags::Output))     // input to input, output to output
    {
        return false;
    }


    if (PinType::COLOR == s1.type && PinType::STRING == s2.type ||    // special case
        PinType::COLOR == s2.type && PinType::STRING == s1.type)
    {
        return true;
    }

    const auto* wf = g::pWfsManager->access(i1->get_workflow_id());
    const auto* n1 = wf->get_node_by_instance(i1->get_parent_node());
    const auto* n2 = wf->get_node_by_instance(i2->get_parent_node());
    LogRAssertS(n1 && n2, "Node not found", false);

    if (n1->flags().has(NodeInstFlags::CNI) && !n1->cni_can_link(i1, i2)) return false;
    if (n2->flags().has(NodeInstFlags::CNI) && !n2->cni_can_link(i2, i1)) return false;

    if (PinType::ANY == i1->state().type || PinType::ANY == i2->state().type)
        return true;

    if (s1.type == s2.type)
    {
        if (PinType::CUSTOM == s1.type)
            return s1.type_str == s2.type_str;
        return true;
    }
    return false;
}

Link* Link::link(PinInstance* i1, PinInstance* i2, iid instance_id /*= 0*/)
{
    LogRAssertS(i1 && i2, "Zero ptr", nullptr);
    LogRAssertS(can_link(i1, i2),
        "Check can link before entablish connection. w1:" << i1->get_workflow_id() << ", p1:" << i1->get_instance_id() << ", w2:" << i2->get_workflow_id() << ", p2:" << i2->get_instance_id(),
        nullptr);

    if (i1->state().links > 0 && i1->obj_flags().has(PinObjFlags::Input) || i2->state().links > 0 && i2->obj_flags().has(PinObjFlags::Input))
    {
        LogRErrorS("Input pin can't has more than 1 links. w1:" << i1->get_workflow_id() << ", p1:" << i1->get_instance_id() << ", w2:" << i2->get_workflow_id() << ", p2:" << i2->get_instance_id(),
            nullptr);
    }

    LogRAssertS(i1->flags().has(PinInstFlags::RerouteSplittingPin) == i2->flags().has(PinInstFlags::RerouteSplittingPin),
        "Both pins must be in the same reroute splitting pins mode. w1:" << i1->get_workflow_id() << ", p1:" << i1->get_instance_id() << ", p2:" << i2->get_instance_id(),
        nullptr);

    auto wf = g::pWfsManager->access(i1->get_workflow_id());
    auto link = wf->add_link(instance_id);

    auto p1 = i1->get_object();
    auto p2 = i2->get_object();

    auto& s1 = i1->state();
    auto& s2 = i2->state();

    NodeInstance* n1 = wf->get_node_by_instance(i1->get_parent_node());
    NodeInstance* n2 = wf->get_node_by_instance(i2->get_parent_node());

    if (p1->flags.has(PinObjFlags::Advanced))
        n1->flags().add(NodeInstFlags::ShowHidden);
    if (p2->flags.has(PinObjFlags::Advanced))
        n2->flags().add(NodeInstFlags::ShowHidden);

    if (i1->flags().has_not(PinInstFlags::RerouteSplittingPin) && !(PinType::ANY == i1->state().type && PinType::ANY == i2->state().type))
    {
        if (PinType::ANY == i1->state().type)
        {
            s2.copy_to(s1);
            i1->state().max = i2->state().max;
            i1->state().min = i2->state().min;
            i1->state().step = i2->state().step;
            p1->def = p2->def;
        }
        else if (PinType::ANY == i2->state().type)
        {
            s1.copy_to(s2);
            i2->state().max = i1->state().max;
            i2->state().min = i1->state().min;
            i2->state().step = i1->state().step;
            p2->def = p1->def;
        }
    }

    s1.links++;
    s2.links++;

    link->m_input = (p1->flags.has(PinObjFlags::Input) ? i1 : i2);
    link->m_output = (p1->flags.has(PinObjFlags::Output) ? i1 : i2);

    if (n1 != nullptr && n1->flags().has(NodeInstFlags::CNI))  n1->cni_pin_linked(i1, i2);
    if (n2 != nullptr && n2->flags().has(NodeInstFlags::CNI))  n2->cni_pin_linked(i2, i1);

    LogS("Linked. w:" << link->get_workflow_id() << ", l:" << link->get_instance_id() << ", p1:" << link->m_input->get_instance_id() << ", p2:" << link->m_output->get_instance_id());
    return link;
}

void Link::unlink()
{
    LogRAssert(m_input && m_output, "Linked pins zero ptr", );
    auto p1 = m_input;
    auto p2 = m_output;

    auto& s1 = p1->state();
    auto& s2 = p2->state();

    LogRAssert(s1.links > 0 && s2.links > 0, "Linked pins have no links", );
    s1.links--;
    s2.links--;

    if (p1->flags().has_not(PinInstFlags::RerouteSplittingPin))
    {
        auto o1 = p1->get_object();
        auto o2 = p2->get_object();

        const auto* wf = g::pWfsManager->access(p1->get_workflow_id());
        auto n1 = wf->get_node_by_instance(p1->get_parent_node());
        auto n2 = wf->get_node_by_instance(p2->get_parent_node());
        LogRAssertS(n1 && n2, "Node not found", );

        data::NodeInstance* recalc = nullptr;
        if (o1->flags.has(PinObjFlags::TypeAny) && 0 == s1.links)
        {
            o1->def_state.copy_to(s1);
            recalc = n1;
        }
        if (o2->flags.has(PinObjFlags::TypeAny) && 0 == s2.links)
        {
            o2->def_state.copy_to(s2);
            recalc = n2;
        }
        if (recalc != nullptr)
            recalc->reset_width();

        if (n1->flags().has(NodeInstFlags::CNI)) n1->cni_pin_unlinked(p1, p2);
        if (n2->flags().has(NodeInstFlags::CNI)) n2->cni_pin_unlinked(p2, p1);
    }

    Log("Unlinked. w:" << get_workflow_id() << ", l:" << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2" << p2->get_instance_id());
}

bool Link::is_reroute_splitting_link() const noexcept
{
    return m_input->flags().has(PinInstFlags::RerouteSplittingPin) || m_output->flags().has(PinInstFlags::RerouteSplittingPin);
}

uint64_t Link::reset_state() noexcept
{
    const uint64_t id = ed().id();
    ed().reset();
    return id;
}

} // namespace data
