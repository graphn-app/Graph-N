
#pragma once
#include "core/workflow/instance.h"

namespace data
{

class PinInstance;
struct Node;

class NodeInstanceBase : public Instance<Node>
{
public:
    using Instance<data::Node>::Instance;
    ~NodeInstanceBase() override;

    void init() override;

    float calc_node_width(bool ro, bool use_big_pin_icon);
    float recalc_node_width(float new_width, bool ro, bool use_big_pin_icon);

    virtual const std::string& get_name() const;
    virtual const std::string& get_name_original() const;
    virtual void set_name(std::string name);

    inline float get_ideal_width() const { return m_ideal_w; }

    inline const std::vector<PinInstance*>& get_inputs() const { return m_input; }
    inline const std::vector<PinInstance*>& get_outputs() const { return m_output; }

    void set_object(Node* obj) override;

protected:
    Node* find_obj(hid obj_id) const override;

    virtual float get_additional_name_width() const { return 0.0f; }
    virtual float get_additional_node_width() const { return 0.0f; }
    virtual bool do_need_calc_node_widths() const { return true; }
    void link_cag_pins();

    void reset_state() override;

public:
    float w_node = 0.0f;
    float w_node_name_off = 0.0f;
    float w_name = -1.0f;
    float w_center = 0.0f;
    float w_separator = 0.0f;
    float w_widget_name = 0.0f;
    float w_widget_value = 0.0f;
    float w_pin_in = 0.0f;
    float w_pin_out = 0.0f;

    bool m_has_in_pins = false;
    bool m_has_out_pins = false;
    bool m_has_widgets = false;

protected:
    std::vector<PinInstance*> m_input;
    std::vector<PinInstance*> m_output;
    std::string m_name;
    float m_saved_width = -1.0f;

    // !!
    // check `init` and `reset_state` when adding new field

private:
    float m_ideal_w = 0.0f;
    float m_min_possible_w = 0.0f;
    const float m_min_center_w = 10.0f;
};

} // namespace data

