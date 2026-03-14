
#include "core/workflow/node_instance_base.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"
#include "core/data/node.h"
#include "core/data/pin.h"

namespace data
{

NodeInstanceBase::~NodeInstanceBase() {};

void NodeInstanceBase::init()
{
    Instance<data::Node>::init();

    if (m_object != nullptr)
    {
        m_input.reserve(m_object->required.pins.size() + m_object->optional.pins.size());
        m_output.reserve(m_object->output.size());
    }
}

void NodeInstanceBase::reset_state()
{
    Instance<data::Node>::reset_state();

    w_node = 0.0f;
    w_node_name_off = 0.0f;
    w_name = -1.0f;
    w_center = 0.0f;
    w_separator = 0.0f;
    w_widget_name = 0.0f;
    w_widget_value = 0.0f;
    w_pin_in = 0.0f;
    w_pin_out = 0.0f;

    m_saved_width = -1.0f;
    m_ideal_w = 0.0f;
    m_min_possible_w = 0.0f;
}

const std::string& NodeInstanceBase::get_name() const
{
    if (m_name.empty() && m_object != nullptr)
        return get_name_original();
    return m_name;
}

const std::string& NodeInstanceBase::get_name_original() const
{
    return m_object->display_name.empty() ? m_object->name : m_object->display_name;
}

void NodeInstanceBase::set_name(std::string name)
{
    if (get_name_original() == name)
        m_name.clear();
    else
        m_name = std::move(name);
    w_name = -1.0f;
    m_saved_width = -1.0f;
    w_node = 0.0f;
}

Node* NodeInstanceBase::find_obj(hid obj_id) const
{
    return g::pNodesManager->find_node(obj_id);
}

void NodeInstanceBase::link_cag_pins()
{
    for (auto it = m_input.begin(); it != m_input.end(); ++it)
    {
        auto pin = *it;
        if (pin->get_object()->cag_require)
        {
            auto cag = *++it;
            if (!cag->get_object()->cag_pin)
            {
                LogInternalLogic("Wrong cag pins order");
            }
            else
            {
                pin->state().cag = cag->get_instance_id();
                cag->state().cag = pin->get_instance_id();
            }
        }
    }
}

float NodeInstanceBase::calc_node_width(bool ro, bool use_big_pin_icon)
{
    m_has_out_pins = !m_output.empty();
    m_object->w_widget_name_max = m_object->w_widget_value_max = m_object->w_pin_in_max = m_object->w_pin_out_max = m_min_possible_w = 0.0f;

    for (auto& in : m_input)
    {
        auto obj = in->get_object();
        if (obj->is_widget())
        {
            m_has_widgets = true;
            m_object->w_widget_name_max = std::max(m_object->w_widget_name_max, in->state().w_name + ImGui::GetStyle().ItemSpacing.x * 2);
            m_object->w_widget_value_max = std::max(m_object->w_widget_value_max, obj->w_value);
        }
        else
        {
            m_has_in_pins = true;
            m_object->w_pin_in_max = std::max(m_object->w_pin_in_max, in->state().w_name);
        }
    }

    for (auto& out : m_output)
    {
        m_object->w_pin_out_max = std::max(m_object->w_pin_out_max, out->state().w_name);
    }

    // empty
    if (!m_has_in_pins && !m_has_out_pins && !m_has_widgets)
    {
        m_object->w_fit = m_min_possible_w = 100.0f;
        m_ideal_w = m_object->w_fit;
        return m_object->w_fit;
    }

    const float min_pins_w = m_object->w_pin_in_max + m_object->w_pin_out_max + m_min_center_w;
    const float min_widget_w = m_object->w_widget_name_max + m_object->w_widget_value_max;
    const float pin_l_offset = ro ? w::pin_l_offset_ro : w::pin_l_offset_rw;
    const float max_ico = (use_big_pin_icon ? w::ico_max : w::ico_circle);
    const float pin_off = (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2);
    m_ideal_w = m_object->w_fit = std::max(min_pins_w, min_widget_w) + pin_off;

    m_min_possible_w = std::max(m_min_possible_w, (m_object->w_widget_name_max + m_object->w_widget_value_max) / 3);
    m_min_possible_w = std::max(m_min_possible_w, m_object->w_pin_in_max + m_object->w_pin_out_max + m_min_center_w);
    m_min_possible_w += pin_off;

    m_saved_width = -1.0f;
    w_node = 0.0f;
    return m_ideal_w;
}

float NodeInstanceBase::recalc_node_width(float new_width, bool ro, bool use_big_pin_icon)
{
    if (ro)
        new_width = m_ideal_w;

    if (w_name <= 0.0f)
    {
        w_name = ImGui::CalcTextSize(get_name().c_str()).x;
        w_name += get_additional_name_width();
    }

    if (m_saved_width == new_width)
        return w_node;

    if (!do_need_calc_node_widths())
    {
        w_node = w_name + ImGui::GetStyle().ItemSpacing.x * 2;
        w_node_name_off = std::max(0.0f, (w_node / 2) - (w_name / 2));
        return w_node;
    }


    new_width = std::max(new_width, m_min_possible_w);
    new_width = std::max(new_width, w_name + ImGui::GetStyle().ItemSpacing.x * 2);

    w_node = new_width;
    w_node_name_off = std::max(0.0f, (new_width / 2) - (w_name / 2));

    const float pin_l_offset = ro ? w::pin_l_offset_ro : w::pin_l_offset_rw;
    const float max_ico = (use_big_pin_icon ? w::ico_max : w::ico_circle);
    const float pin_off = (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2);

    if (new_width < m_ideal_w)
    {
        const auto avail_w = new_width - pin_off;
        w_widget_name = avail_w / 3;
        w_widget_value = avail_w / 3 * 2;
        w_pin_in = (avail_w - m_min_center_w) / 2;
        w_pin_out = (avail_w - m_min_center_w) / 2;
        w_center = m_min_center_w;
    }
    else if (new_width > m_ideal_w)
    {
        w_node = new_width;
        w_widget_name = m_object->w_widget_name_max;
        w_widget_value = w_node - w_widget_name - pin_off;
        w_pin_in = m_object->w_pin_in_max;
        w_pin_out = m_object->w_pin_out_max;
        w_center = w_node - pin_off - w_pin_in - w_pin_out;
    }
    else
    {
        w_node = m_ideal_w;
        w_widget_name = m_object->w_widget_name_max;
        w_widget_value = m_object->w_widget_value_max;
        w_pin_in = m_object->w_pin_in_max;
        w_pin_out = m_object->w_pin_out_max;
        w_center = w_node - pin_off - w_pin_in - w_pin_out;
    }

    m_saved_width = new_width;
    return w_node;
}

} // namespace data
