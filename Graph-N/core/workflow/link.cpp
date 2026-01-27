
#include "core/workflow/link.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/data/pin.h"
#include "core/managers/nodes_manager.h"

namespace data
{

Link::Link(Workflow* wf, gid graph_id)
    : m_wf(wf)
    , m_graph_id(graph_id)
{}

Link::~Link()
{}

bool Link::can_link(const PinInstance* i1, const PinInstance* i2)
{
    if (nullptr == i1 || nullptr == i2 || i1->get_object() == nullptr || i2->get_object() == nullptr)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Null link");
    }

    if (i1->get_workflow() != i2->get_workflow())
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Different workflow");
    }

    const auto p1 = i1->get_object();
    const auto p2 = i2->get_object();

    if (i1 == i2 || i1->get_parent_instance() == i2->get_parent_instance())
        return false;

    const auto& s1 = i1->m_state;
    const auto& s2 = i2->m_state;

    if (p1->input == p2->input || p1->output == p2->output)
        return false;
    if (p1->type_any && p2->type_any)
        return false;
    if (p1->input && s1->links > 0 || p2->input && s2->links > 0)
        return false;

    if (PinType::ANY == s1->type || PinType::ANY == s2->type)
        return true;

    if (PinType::COLOR == s1->type && PinType::STRING == s2->type ||
        PinType::COLOR == s2->type && PinType::STRING == s1->type)
    {
        return true;
    }

    return (
        s1->type == s2->type
        &&
        p1->hash == p2->hash
    );
}

Link* Link::link(PinInstance* i1, PinInstance* i2)
{
    if (!can_link(i1, i2))
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Check can link before entablish connection");
    }

    auto link = i1->get_workflow()->add_link();

    auto p1 = i1->get_object();
    auto p2 = i2->get_object();

    auto& s1 = i1->m_state;
    auto& s2 = i2->m_state;

    if (p1->type_any)
    {
        s1->type = s2->type;
        s1->type_str = s2->type_str;
        s1->pin_color = s2->pin_color;
    }
    else if (p2->type_any)
    {
        s2->type = s1->type;
        s2->type_str = s1->type_str;
        s2->pin_color = s1->pin_color;
    }

    s1->links++;
    s2->links++;

    if (p1->input)
    {
        link->m_input = i1;
        link->m_output = i2;
    }
    else
    {
        link->m_input = i2;
        link->m_output = i1;
    }

    Log("Link+: link id=" << link->m_graph_id << ", pin id=" << link->m_input->get_graph_id() << "/" << p1->get_id() << " linked to id=" << link->m_output->get_graph_id() << "/" << p2->get_id());
    return link;
}

void Link::unlink()
{
    auto p1 = m_input->get_object();
    auto p2 = m_output->get_object();

    auto& s1 = m_input->m_state;
    auto& s2 = m_output->m_state;

    s1->links--;
    s2->links--;

    if (p1->type_any && 0 == s1->links)
    {
        s1->type = p1->def_state.type;
        s1->type_str = p1->def_state.type_str;
        s1->pin_color = p1->def_state.pin_color;
    }
    if (p2->type_any && 0 == s2->links)
    {
        s2->type = p2->def_state.type;
        s2->type_str = p2->def_state.type_str;
        s2->pin_color = p2->def_state.pin_color;
    }

    Log("Link-: link id=" << m_graph_id << ", pin id=" << m_input->get_graph_id() << "/" << p1->get_id() << " unlinked from id=" << m_output->get_graph_id() << "/" << p2->get_id());
}

} // namespace data
