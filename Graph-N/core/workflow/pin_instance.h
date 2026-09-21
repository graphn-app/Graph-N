#pragma once
#include "core/workflow/instance.h"

namespace data
{
class NodeInstanceBase;
struct Pin;
struct PinState;

class PinInstance final : public Instance<Pin, PinInstFlags, PinObjFlags>
{
public:
    PinInstance(NodeInstanceBase* parent_node, Pin* obj, wid workflow_id, iid instance_id) noexcept;
    ~PinInstance() noexcept override;

    void init() override;

    iid get_parent_node() const noexcept { return m_parent_node; }

    PinState& state() noexcept { return m_state; }
    const PinState& state() const noexcept { return m_state; }

    void set_object(Pin* obj) override;
    void set_name(const std::string& name);

    bool is_multiline() const noexcept
    {
        if (flags().has(PinInstFlags::MultilineSet))
            return flags().has(PinInstFlags::Multiline);
        return obj_flags().has(PinObjFlags::Multiline);
    }

private:
    PinState m_state;
    iid m_parent_node = 0;
};

} // namespace data
