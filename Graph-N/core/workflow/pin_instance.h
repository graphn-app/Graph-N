
#pragma once
#include "core/workflow/instance.h"

namespace data
{
class NodeInstanceBase;
struct Pin;
struct PinState;

class PinInstance final : public Instance<Pin>
{
    friend class WorkflowSerializerToBlob;
    friend class WorkflowSerializerToJson;

public:
    PinInstance(NodeInstanceBase* parent_node, Pin* obj, wid workflow_id, iid instance_id);
    ~PinInstance() override;

    void init();

    inline iid get_parent_node() const { return m_parent_node; }

    inline PinState& state() { return m_state; }
    inline const PinState& state() const { return m_state; }

    void set_object(Pin* obj) override;
    void set_name(const std::string& name);

private:
    PinState m_state;    
    iid m_parent_node = 0;
    hid m_parent_obj_hid = 0;

    bool m_obj_in = true;
    int64_t m_obj_out_idx = -1;
};

} // namespace data
