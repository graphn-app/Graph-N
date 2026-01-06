
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

protected:
    data::Pin* find_obj(hid obj_id) const override;

public:
    std::unique_ptr<PinState> m_state;
};

} // namespace data
