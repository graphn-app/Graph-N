#include "core/workflow/node_instance_ro.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/workflows_manager.h"

namespace data
{

NodeInstanceRo::~NodeInstanceRo() noexcept = default;

void NodeInstanceRo::init()
{
    NodeInstanceBase::init();

    auto wf = g::pWfsManager->access(m_wid);

    const auto add_pins = [this, wf](const std::vector<data::Pin*>& source, std::vector<PinInstance*>& dest)
    {
        for (auto pin : source)
        {
            auto pin_instance = wf->add_pin(this, pin);
            dest.push_back(pin_instance);
        }
    };

    add_pins(get_object()->input, m_input);
    add_pins(get_object()->output, m_output);

    link_cag_pins();
}

} // namespace data
