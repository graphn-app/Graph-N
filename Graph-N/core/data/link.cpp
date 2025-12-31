
#include "core/data/link.h"
#include "core/data/pin.h"
#include "ui/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"

namespace data
{

Link::Link()
{}

Link::~Link()
{}

bool Link::can_link(const Pin* p1, const Pin* p2)
{
    if (nullptr == p1 || nullptr == p2)
    {
        LogAndThrowInternalError(ExcepCategory::logic, "Null link");
    }

    if (p1->input == p2->input || p1->output == p2->output)
        return false;
    if (p1->type_any && p2->type_any)
        return false;
    if (p1->input && p1->links > 0 || p2->input && p2->links > 0)
        return false;

    if (p1->type_any || p2->type_any)
        return true;

    if (PinType::COLOR == p1->type && PinType::STRING == p1->type ||
        PinType::COLOR == p2->type && PinType::STRING == p1->type)
    {
        return true;
    }

    return (
        p1->type == p2->type
        &&
        p1->hash == p2->hash
    );
}

Link* Link::link(const Pin* p1, const Pin* p2)
{
    if (!can_link(other))
    {
        LogAndThrowInternalError(ExcepCategory::logic, "Check can link before entablish connection");
    }

    if (type_any)
    {
        type = other->type;
        type_str = other->type_str;
        pin_color = other->pin_color;
    }
    else if (other->type_any)
    {
        other->type = type;
        other->type_str = type_str;
        other->pin_color = pin_color;
    }
    links.push_back(other);
    other->links.push_back(this);
    Log("Pin " << get_id() << " linked to " << other->get_id());
}

void Link::unlink(Pin* other)
{
    if (nullptr == other)
    {
        LogAndThrowInternalError(ExcepCategory::logic, "Null link");
    }
    if (type_any && links.size() == 1)
    {
        type = PinType::ANY;
        type_str = "*";
        pin_color = IM_COL32_WHITE;
    }
    else if (other->type_any && other->links.size() == 1)
    {
        other->type = PinType::ANY;;
        other->type_str = "*";
        other->pin_color = IM_COL32_WHITE;
    }

    links.remove(other);
    other->links.remove(this);

    Log("Pin " << get_id() << " unlinked from " << other->get_id());

}

void Link::invalidate_comfyui_objects()
{
    validation_state = true;
    links_id.clear();
    for (auto pin : links)
    {
        unlink(pin);
        links_id.push_back(pin->get_id());
    }
    links.clear();
}

void Link::resolve_comfyui_objects()
{
    for (auto id : links_id)
    {
        auto pin = g::pNodesManager->find_pin(id);
        if (pin != nullptr && can_link(pin))
            link_to(pin);
    }

    validation_state = false;
    links_id.clear();
}

} // namespace data
