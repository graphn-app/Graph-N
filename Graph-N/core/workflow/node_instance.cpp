
#include "core/workflow/node_instance.h"
#include "core/workflow/workflow.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/settings_manager.h"
#include "ui/graph-n_flow.h"

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

const std::string& NodeInstance::get_name() const
{
    if (m_name.empty())
        return m_object->display_name.empty() ? m_object->name : m_object->display_name;
    return m_name;
}

void NodeInstance::set_name(std::string name)
{
    if (m_object->name == name)
        m_name.clear();
    else
        m_name = std::move(name);
    w_name = -1.0f;
}

void NodeInstance::add_pin()
{
    m_saved_width = 0.0f;
}

void NodeInstance::remove_pin()
{
    m_saved_width = 0.0f;
}

float NodeInstance::calc_node_width()
{
    //if (m_object->w_ready)
    //    return m_object->w_fit;

    m_has_out_pins = !m_output.empty();

    const auto widget_name_add = ImGui::GetStyle().ItemSpacing.x * 2;
    for (auto& in : m_input)
    {
        auto obj = in->get_object();
        if (obj->is_widget())
        {
            m_has_widgets = true;
            m_object->w_widget_name_max = std::max(m_object->w_widget_name_max, obj->w_name + widget_name_add);
            m_object->w_widget_value_max = std::max(m_object->w_widget_value_max, obj->w_value);
        }
        else
        {
            m_has_in_pins = true;
            m_object->w_pin_in_max = std::max(m_object->w_pin_in_max, obj->w_name);
        }
    }

    for (auto& out : m_output)
    {
        m_object->w_pin_out_max = std::max(m_object->w_pin_out_max, out->get_object()->w_name);
    }

    if (!m_has_in_pins && !m_has_out_pins && !m_has_widgets)
    {
        m_object->w_fit = 100.0f;
    }
    else if (!m_has_in_pins && !m_has_out_pins && m_has_widgets)
    {
        m_object->w_fit = (w::pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_l_offset * 2) + m_object->w_widget_name_max + m_object->w_widget_value_max;
    }
    else if (m_has_in_pins && !m_has_out_pins && !m_has_widgets)
    {
        m_object->w_fit = (w::pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_l_offset * 2) + m_object->w_pin_in_max + 50.0f;
    }
    else if (!m_has_in_pins && m_has_out_pins && !m_has_widgets)
    {
        m_object->w_fit = (w::pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_l_offset * 2) + 50.0f + m_object->w_pin_out_max;
    }
    else if (!m_has_in_pins && m_has_out_pins && m_has_widgets)
    {
        m_object->w_fit = (w::pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_l_offset * 2) + m_object->w_widget_name_max + m_object->w_widget_value_max;
    }
    else if (m_has_in_pins && !m_has_out_pins && m_has_widgets)
    {
        m_object->w_fit = (w::pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_l_offset * 2) + m_object->w_widget_name_max + m_object->w_widget_value_max;
    }
    else if (m_has_in_pins && m_has_out_pins && !m_has_widgets)
    {
        m_object->w_fit = (w::pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_l_offset * 2) + m_object->w_pin_in_max + 10.0f + m_object->w_pin_out_max;
    }
    else
    {
        m_object->w_fit = (w::pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_r_offset * 2) + m_object->w_widget_name_max + m_object->w_widget_value_max;
    }

    //m_object->w_ready = true;
    return m_object->w_fit;
}

float NodeInstance::recalc_node_widths(float avail_width, bool allow_extend, bool allow_compress, bool use_big_pin_icon)
{
    if (w_name <= 0.0f)
    {
        if (m_name.empty())
            w_name = m_object->w_name;
        else
            w_name = ImGui::CalcTextSize(m_name.c_str()).x;
    }
    if (m_saved_width == avail_width)
        return w_node;

    const float max_ico = (use_big_pin_icon ? w::ico_max : w::ico_circle);

    if (avail_width < m_object->w_fit && allow_compress)
    {
        w_node = avail_width;
        w_node_name_off = std::max(0.0f, (avail_width / 2) - (w_name / 2));
        w_widget_name = std::max(0.0f, (avail_width - (w::pin_l_offset * 2) - (max_ico * 2) - (w::pin_l_offset * 2)) / 3);
        w_widget_value = std::max(0.0f, (avail_width - (w::pin_l_offset * 2) - (max_ico * 2) - (w::pin_l_offset * 2)) / 3 * 2);
        w_pin_in = (w_widget_name + w_widget_value) / 2;
        w_pin_out = (w_widget_name + w_widget_value) / 2;
        w_center = std::max(0.0f, w_widget_name + w_widget_value - w_pin_in - w_pin_out);
    }
    else if (avail_width > m_object->w_fit && allow_extend)
    {
        w_node = avail_width;
        w_node_name_off = (avail_width / 2) - (w_name / 2);
        w_widget_name = m_object->w_widget_name_max;
        w_widget_value = std::max(0.0f, avail_width - (w::pin_l_offset * 2) - (max_ico * 2) - (w::pin_l_offset * 2) - w_widget_name);
        w_pin_in = m_object->w_pin_in_max;
        w_pin_out = m_object->w_pin_out_max;
        w_center = std::max(0.0f, w_widget_name + w_widget_value - w_pin_in - w_pin_out);
    }
    else
    {
        w_node = m_object->w_fit;
        w_node_name_off = (w_node / 2) - (w_name / 2);
        w_widget_name = m_object->w_widget_name_max;
        w_widget_value = m_object->w_widget_value_max;
        w_pin_in = m_object->w_pin_in_max;
        w_pin_out = m_object->w_pin_out_max;
        w_center = std::max(0.0f, w_widget_name + w_widget_value - w_pin_in - w_pin_out);
    }

    if (!m_has_in_pins && !m_has_out_pins && !m_has_widgets)
    {
        w_node = (allow_extend && avail_width > m_object->w_fit || allow_compress && avail_width < m_object->w_fit ? avail_width : m_object->w_fit);
    }
    else if (!m_has_in_pins && !m_has_out_pins && m_has_widgets)
    {
        w_node = (w::pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_widget_name + w_widget_value;
    }
    else if (m_has_in_pins && !m_has_out_pins && !m_has_widgets)
    {
        w_center = 50.0f;
        w_node = (w::pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_pin_in + w_center;
    }
    else if (!m_has_in_pins && m_has_out_pins && !m_has_widgets)
    {
        w_center = 50.0f;
        w_node = (w::pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_center + w_pin_out;
    }
    else if (!m_has_in_pins && m_has_out_pins && m_has_widgets)
    {
        w_node = (w::pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_widget_name + w_widget_value;
        w_center = w_widget_name + w_widget_name - w_pin_out;
    }
    else if (m_has_in_pins && !m_has_out_pins && m_has_widgets)
    {
        w_node = (w::pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_widget_name + w_widget_value;
        w_center = w_widget_name + w_widget_name - w_pin_in;
    }
    else if (m_has_in_pins && m_has_out_pins && !m_has_widgets)
    {
        w_center = 10.0f;
        w_node = (w::pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_pin_in + w_center + w_pin_out;
    }

    w_separator = std::max(0.0f, w_node - (w::pin_l_offset * 2) - (max_ico * 2) - (w::pin_r_offset * 2));

    m_saved_width = avail_width;
    return w_node;
}

Node* NodeInstance::find_obj(hid obj_id) const
{
    return g::pNodesManager->find_node(obj_id);
}

} // namespace data
