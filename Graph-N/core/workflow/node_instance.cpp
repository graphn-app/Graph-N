
#include "core/workflow/node_instance.h"
#include "core/workflow/workflow.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/workflows_manager.h"

namespace data
{

NodeInstance::~NodeInstance() {};

void NodeInstance::init(const std::string& select)
{
    Instance<data::Node>::init();

    m_input.reserve(m_object->required.pins.size() + m_object->optional.pins.size());
    m_output.reserve(m_object->output.size());

    auto wf = g::pWfsManager->access(m_wid);

    const auto add_pins = [this, wf, &select](const std::vector<data::Pin*>& source, std::vector<PinInstance*>& dest)
    {
        for (auto& pin : source)
        {
            auto pin_instance = wf->add_pin(this, pin);
            if (!select.empty())
            {
                const auto& ptrs = pin_instance->get_object()->combo_items_ptr;
                for (int32_t i = 0; i < ptrs.size(); i++)
                {
                    if (ptrs[i] == select)
                    {
                        pin_instance->m_state->combo_index = i;
                        break;
                    }
                }
            }
            dest.push_back(pin_instance);
        }
    };

    add_pins(m_object->required.pins, m_input);
    add_pins(m_object->optional.pins, m_input);
    add_pins(m_object->hidden.pins, m_input);
    add_pins(m_object->output, m_output);
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

float NodeInstance::calc_node_width(bool ro)
{
    const float pin_l_offset = ro ? w::pin_l_offset_ro : w::pin_l_offset_rw;

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
        m_object->w_fit = (pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_r_offset * 2) + m_object->w_widget_name_max + m_object->w_widget_value_max;
    }
    else if (m_has_in_pins && !m_has_out_pins && !m_has_widgets)
    {
        m_object->w_fit = (pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_r_offset * 2) + m_object->w_pin_in_max + 50.0f;
    }
    else if (!m_has_in_pins && m_has_out_pins && !m_has_widgets)
    {
        m_object->w_fit = (pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_r_offset * 2) + 50.0f + m_object->w_pin_out_max;
    }
    else if (!m_has_in_pins && m_has_out_pins && m_has_widgets)
    {
        m_object->w_fit = (pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_r_offset * 2) + m_object->w_widget_name_max + m_object->w_widget_value_max;
    }
    else if (m_has_in_pins && !m_has_out_pins && m_has_widgets)
    {
        m_object->w_fit = (pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_r_offset * 2) + m_object->w_widget_name_max + m_object->w_widget_value_max;
    }
    else if (m_has_in_pins && m_has_out_pins && !m_has_widgets)
    {
        m_object->w_fit = (pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_r_offset * 2) + m_object->w_pin_in_max + 10.0f + m_object->w_pin_out_max;
    }
    else
    {
        m_object->w_fit = (pin_l_offset * 2) + (w::ico_max * 2) + (w::pin_r_offset * 2) + m_object->w_widget_name_max + m_object->w_widget_value_max;
    }

    //m_object->w_ready = true;
    return m_object->w_fit;
}

float NodeInstance::recalc_node_width(bool ro, float avail_width, bool allow_extend, bool allow_compress, bool use_big_pin_icon)
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

    const float pin_l_offset = ro ? w::pin_l_offset_ro : w::pin_l_offset_rw;

    const float max_ico = (use_big_pin_icon ? w::ico_max : w::ico_circle);
    float min_w = w_name + ImGui::GetStyle().ItemSpacing.x * 2;
    min_w = std::max(min_w, m_object->w_pin_in_max + m_object->w_pin_out_max + 20.0f + (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2));
    min_w = std::max(min_w, m_object->w_widget_name_max + m_object->w_widget_value_max);

    if (allow_extend)
        min_w = std::max(avail_width, min_w);

    if (avail_width < m_object->w_fit && allow_compress)
    {
        w_node = avail_width;
        w_node_name_off = std::max(0.0f, (avail_width / 2) - (w_name / 2));
        w_widget_name = std::max(0.0f, (avail_width - (pin_l_offset * 2) - (max_ico * 2) - (w::pin_r_offset * 2)) / 3);
        w_widget_value = std::max(0.0f, (avail_width - (pin_l_offset * 2) - (max_ico * 2) - (w::pin_r_offset * 2)) / 3 * 2);
        w_pin_in = (w_widget_name + w_widget_value) / 2;
        w_pin_out = (w_widget_name + w_widget_value) / 2;
        w_center = std::max(0.0f, w_widget_name + w_widget_value - w_pin_in - w_pin_out);
    }
    else if (avail_width > m_object->w_fit && allow_extend)
    {
        w_node = avail_width;
        w_node_name_off = std::max(0.0f, (avail_width / 2) - (w_name / 2));
        w_widget_name = m_object->w_widget_name_max;
        w_widget_value = std::max(0.0f, avail_width - (pin_l_offset * 2) - (max_ico * 2) - (w::pin_r_offset * 2) - w_widget_name);
        w_pin_in = m_object->w_pin_in_max;
        w_pin_out = m_object->w_pin_out_max;
        w_center = std::max(0.0f, w_widget_name + w_widget_value - w_pin_in - w_pin_out);
    }
    else
    {
        w_node = m_object->w_fit;
        w_node = std::max(min_w, w_node);
        w_node_name_off = std::max(0.0f, (w_node / 2) - (w_name / 2));
        w_widget_name = m_object->w_widget_name_max;
        w_widget_value = m_object->w_widget_value_max;
        w_pin_in = m_object->w_pin_in_max;
        w_pin_out = m_object->w_pin_out_max;
        w_center = std::max(20.0f, w_widget_name + w_widget_value - w_pin_in - w_pin_out);

        if (w_widget_name + w_widget_value + (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) < w_node)
        {
            const auto prev = w_widget_value;
            w_widget_value = w_node - w_widget_name - (pin_l_offset * 2) - (max_ico * 2) - (w::pin_r_offset * 2);
            w_center += w_widget_value - prev;
        }
    }

    // empty
    if (!m_has_in_pins && !m_has_out_pins && !m_has_widgets)
    {
        w_node = (allow_extend && avail_width > m_object->w_fit || allow_compress && avail_width < m_object->w_fit ? avail_width : m_object->w_fit);
    }
    // widgets only
    else if (!m_has_in_pins && !m_has_out_pins && m_has_widgets)
    {
        w_node = (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_widget_name + w_widget_value;
    }
    // input only
    else if (m_has_in_pins && !m_has_out_pins && !m_has_widgets)
    {
        w_center = 50.0f;
        w_node = (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_pin_in + w_center;
    }
    // output only
    else if (!m_has_in_pins && m_has_out_pins && !m_has_widgets)
    {
        w_center = 50.0f;
        w_node = (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_center + w_pin_out;
    }
    // no input
    else if (!m_has_in_pins && m_has_out_pins && m_has_widgets)
    {
        w_center = w_widget_name + w_widget_value - w_pin_out;
        w_node = (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_widget_name + w_widget_value;
    }
    // no output
    else if (m_has_in_pins && !m_has_out_pins && m_has_widgets)
    {
        w_center = w_widget_name + w_widget_value - w_pin_in;
        w_node = (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_widget_name + w_widget_value;
    }
    // no widgets
    else if (m_has_in_pins && m_has_out_pins && !m_has_widgets)
    {
        w_center = 20.0f;
        w_node = (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2) + w_pin_in + w_center + w_pin_out;  
    }

    if (w_node < min_w)
    {
        w_center += min_w - w_node;
        w_node = min_w;
    }

    w_separator = std::max(0.0f, w_node - (pin_l_offset * 2) - (max_ico * 2) - (w::pin_r_offset * 2));

    m_saved_width = avail_width;
    return w_node;
}

Node* NodeInstance::find_obj(hid obj_id) const
{
    return g::pNodesManager->find_node(obj_id);
}

} // namespace data
