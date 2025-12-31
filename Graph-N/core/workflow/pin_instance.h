
#pragma once
#include "core/workflow/instance.h"

namespace data
{
struct Pin;
struct PinState;

class PinInstance final : public Instance<data::Pin>
{
public:
    using Instance<data::Pin>::Instance;
    ~PinInstance() override;

    void init() override;

    PinState* get_state() { return m_state.get(); }
    const PinState* get_state() const { return m_state.get(); }

protected:
    data::Pin* find_obj(hid obj_id) override;

private:
    std::unique_ptr<PinState> m_state;
};

} // namespace data
