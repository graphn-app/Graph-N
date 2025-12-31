
#pragma once
#include "ui/workflow/instance.h"
#include "core/data/pin.h"

namespace data
{
struct Pin;
}

namespace ui
{

class PinInstance final : public Instance<data::Pin>
{
public:
    using Instance<data::Pin>::Instance;
    ~PinInstance() override;

    void draw() override;
    void init() override;

protected:
    void make_graph_id() override;
    data::Pin* find_obj(hid obj_id) override;
};

} // namespace ui
