
#pragma once
#include "core/workflow/instance.h"

namespace data
{
class NodeInstanceBase;
struct Pin;
struct PinState;

class PinInstance final : public Instance<Pin>
{
public:
    PinInstance(NodeInstanceBase* parent_node, Pin* obj, wid workflow_id, iid instance_id) noexcept;
    ~PinInstance() noexcept override;

    void init() override;

    iid get_parent_node() const noexcept { return m_parent_node; }

    void set_is_reroute_splitting_pin() noexcept { m_reroute_splitting_pin = true; }
    bool get_is_reroute_splitting_pin() const noexcept { return m_reroute_splitting_pin; }

    PinState& state() noexcept { return m_state; }
    const PinState& state() const noexcept { return m_state; }

    void set_object(Pin* obj) override;
    void set_name(const std::string& name);

private:
    PinState m_state;
    iid m_parent_node = 0;
    bool m_reroute_splitting_pin = false;
};

} // namespace data
