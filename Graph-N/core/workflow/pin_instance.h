
#pragma once
#include "core/workflow/instance.h"

namespace data
{
struct Pin;
struct PinState;

class PinInstance final : public Instance<Pin>
{
public:
    PinInstance(iid parent_node, Pin* obj, wid workflow_id, iid instance_id);
    ~PinInstance() override;

    void init() override;

    inline iid get_parent_node() const { return m_parent_node; }

protected:
    data::Pin* find_obj(hid obj_id) const override;

public:
    std::unique_ptr<PinState> m_state;
    iid m_parent_node = 0;
};

} // namespace data
