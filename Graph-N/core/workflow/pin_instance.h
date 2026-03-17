
#pragma once
#include "core/workflow/instance.h"

namespace data
{
struct Pin;
struct PinState;

class PinInstance final : public Instance<Pin>
{
    friend class WorkflowSerializerToBlob;

public:
    PinInstance(iid parent_node, Pin* obj, wid workflow_id, iid instance_id);
    ~PinInstance() override;

    void init();

    inline iid get_parent_node() const { return m_parent_node; }

    inline PinState& state() { return m_state; }
    inline const PinState& state() const { return m_state; }

protected:
    data::Pin* find_obj(hid obj_id) const override;

private:
    PinState m_state;
    iid m_parent_node = 0;
};

} // namespace data
