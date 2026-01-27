
#pragma once
#include "core/workflow/instance.h"

namespace data
{
struct Pin;
struct PinState;
class NodeInstance;

class PinInstance final : public Instance<Pin>
{
public:
    PinInstance(Workflow* wf, Pin* obj, NodeInstance* parent_instance, iid instance_id, gid graph_id);
    ~PinInstance() override;

    void init() override;

    NodeInstance* get_parent_instance() const { return m_parent_instance; }

protected:
    data::Pin* find_obj(hid obj_id) const override;

public:
    std::unique_ptr<PinState> m_state;
    NodeInstance* m_parent_instance = nullptr;
};

} // namespace data
