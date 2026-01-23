
#pragma once
#include "core/workflow/instance.h"

namespace data
{

class Workflow;
class PinInstance;
struct Node;

class NodeInstance final : public Instance<Node>
{
public:
    using Instance<data::Node>::Instance;
    ~NodeInstance() override;

    void init() override;

    float calc_node_width();
    float recalc_node_width(float avail_width, bool allow_extend, bool allow_compress, bool use_big_pin_icon);

    const std::string& get_name() const;
    void set_name(std::string name);

    // reroute
    void add_pin();
    void remove_pin();

protected:
    Node* find_obj(hid obj_id) const override;

public:
    std::vector<PinInstance*> m_input;
    std::vector<PinInstance*> m_output;

    float w_node = 0.0f;
    float w_node_name_off = 0.0f;
    float w_name = -1.0f;
    float w_widget_name = 0.0f;
    float w_widget_value = 0.0f;
    float w_center = 0.0f;
    float w_pin_in = 0.0f;
    float w_pin_out = 0.0f;
    float w_separator = 0.0f;

    bool m_has_in_pins = false;
    bool m_has_out_pins = false;
    bool m_has_widgets = false;

private:
    float m_saved_width = -1.0f;
    std::string m_name;
};

} // namespace data
