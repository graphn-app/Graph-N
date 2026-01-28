
#include "core/workflow/link.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/workflows_manager.h"

namespace data
{

Link::Link(wid workflow_id, iid link_id)
    : m_workflow_id(workflow_id)
    , m_iid(link_id)
{}

Link::~Link()
{}

bool Link::can_link(const PinInstance* i1, const PinInstance* i2)
{
    if (nullptr == i1 || nullptr == i2 || i1->get_object() == nullptr || i2->get_object() == nullptr)
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Null link");
    }

    if (i1->get_workflow_id() != i2->get_workflow_id())
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Different workflow");
    }

    const auto p1 = i1->get_object();
    const auto p2 = i2->get_object();

    if (i1 == i2 || i1->get_parent_node() == i2->get_parent_node())
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

    auto wf = g::pWfsManager->access(i1->get_workflow_id());
    auto link = wf->add_link();

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

    link->m_input = (p1->input ? i1 : i2);
    link->m_output = (p1->output ? i1 : i2);

    Log("Linked: wid=" << link->m_workflow_id << ", iid=" << link->m_iid << ", in=" << link->m_input << ", out=" << link->m_output);
    return link;
}

void Link::unlink()
{
    auto p1 = m_input;
    auto p2 = m_output;

    auto& s1 = p1->m_state;
    auto& s2 = p2->m_state;

    s1->links--;
    s2->links--;

    auto o1 = p1->get_object();
    auto o2 = p2->get_object();

    if (o1->type_any && 0 == s1->links)
    {
        s1->type = o1->def_state.type;
        s1->type_str = o1->def_state.type_str;
        s1->pin_color = o1->def_state.pin_color;
    }
    if (o2->type_any && 0 == s2->links)
    {
        s2->type = o2->def_state.type;
        s2->type_str = o2->def_state.type_str;
        s2->pin_color = o2->def_state.pin_color;
    }

    Log("Unlinked: wid=" << m_workflow_id << ", iid=" << m_iid << ", in=" << m_input << ", out=" << m_output);
}

} // namespace data
