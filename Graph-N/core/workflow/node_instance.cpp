
#include "core/workflow/node_instance.h"
#include "core/workflow/workflow.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"

namespace data
{

NodeInstance::~NodeInstance() {};

void NodeInstance::init()
{
    Instance<data::Node>::init();

    m_input.reserve(m_object->required.pins.size() + m_object->optional.pins.size());
    m_output.reserve(m_object->output.size());

    const auto add_pins = [](Workflow* wf, const std::vector<data::Pin*>& source, std::vector<PinInstance*>& dest)
    {
        for (auto& pin : source)
        {
            dest.push_back(wf->add_pin(pin));
        }
    };

    add_pins(m_wf, m_object->required.pins, m_input);
    add_pins(m_wf, m_object->optional.pins, m_input);
    add_pins(m_wf, m_object->hidden.pins, m_input);
    add_pins(m_wf, m_object->output, m_output);
}

data::Node* NodeInstance::find_obj(hid obj_id)
{
    return g::pNodesManager->find_node(obj_id);
}

void NodeInstance::set_node_width(float node_width, bool allow_unlimited_w)
{
    if (node_width > m_object->node_width)
    {
        if (!allow_unlimited_w)
        {
            m_node_w = m_object->node_width;
            m_right_group_w = m_object->right_group_width;
        }
        else
        {
            m_node_w = node_width;
            m_right_group_w = node_width - m_object->left_group_width;
        }
    }
    else
    {
        m_node_w = std::max(get_node_min_width(), node_width);
        m_right_group_w = m_node_w / 3 * 2;
    }
}

} // namespace data
