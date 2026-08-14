
#include "core/workflow/node_instance_base.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"
#include "core/workflow/node.h"
#include "core/workflow/pin.h"

namespace data
{

NodeInstanceBase::~NodeInstanceBase() noexcept = default;

void NodeInstanceBase::init()
{
    Instance<data::Node>::init();

    if (get_object() != nullptr)
    {
        m_input.reserve(get_object()->input.size());
        m_output.reserve(get_object()->output.size());
    }
}

void NodeInstanceBase::reset_width() noexcept
{
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

    calc_node_width(false, false);
    m_saved_width = -1.0f;
    w_node = 0.0f;
    w_name = -1.0f;
}

uint64_t NodeInstanceBase::reset_state() noexcept
{
   const auto id = Instance<data::Node>::reset_state();
   reset_width();
   return id;
}

const std::string& NodeInstanceBase::get_name() const noexcept
{
    if (m_name.empty())
        return get_name_original();
    return m_name;
}

const std::string& NodeInstanceBase::get_name_original() const noexcept
{
    auto obj = get_object();
    if (nullptr == obj)
    {
        static std::string empty;
        return empty;
    }
    return obj->display_name.empty() ? obj->name : obj->display_name;
}

void NodeInstanceBase::set_name(std::string name) noexcept
{
    if (get_name_original() == name)
        m_name.clear();
    else
        m_name = std::move(name);
    w_name = -1.0f;
    m_saved_width = -1.0f;
    w_node = 0.0f;
}

void NodeInstanceBase::link_cag_pins() noexcept
{
    if (get_object() != nullptr)
    {
        for (auto it = m_input.begin(); it != m_input.end(); ++it)
        {
            auto pin = *it;
            if (pin->get_object()->cag_require)
            {
                auto cag = *++it;
                LogCAssert(cag->get_object()->cag_pin, "Wrong cag pins order. w:" << get_workflow_id() << ", n:" << get_instance_id());

                pin->state().cag = cag->get_instance_id();
                cag->state().cag = pin->get_instance_id();
            }
        }
    }
}

std::list<PinType> NodeInstanceBase::get_supported_pin_types(PinInstance* pin) const
{
    LogRAssert(pin, "Zero ptr", std::list<PinType>{});
    return { pin->state().type };
}

void NodeInstanceBase::add_input(PinInstance* pin)
{
    m_input.push_back(pin);
    reset_width();
}

void NodeInstanceBase::add_output(PinInstance* pin)
{
    m_output.push_back(pin);
    reset_width();
}

void NodeInstanceBase::reset_pins() noexcept
{
    m_input.clear(); m_input.shrink_to_fit();
    m_output.clear(); m_output.shrink_to_fit();
    reset_width();
}

void NodeInstanceBase::remove_pin(const PinInstance* pin) noexcept
{
    for (auto it = m_input.begin(); it != m_input.end(); ++it)
    {
        auto in = *it;
        if (in == pin)
        {
            m_input.erase(it);
            break;
        }
    }
    for (auto it = m_output.begin(); it != m_output.end(); ++it)
    {
        auto out = *it;
        if (out == pin)
        {
            m_output.erase(it);
            break;
        }
    }
    reset_width();
}

float NodeInstanceBase::calc_node_width(bool ro, bool use_big_pin_icon) noexcept
{
    auto obj = get_object();
    if (nullptr == obj) return 0.0f;
    
    m_has_out_pins = m_has_in_pins = m_has_widgets = false;
    obj->w_widget_name_max = obj->w_widget_value_max = obj->w_pin_in_max = get_object()->w_pin_out_max = m_min_possible_w = 0.0f;

    for (auto& in : m_input)
    {
        auto pin_obj = in->get_object();
        if (pin_obj->is_widget(in->state()))
        {
            m_has_widgets = true;
            in->state().w_name = ImGui::CalcTextSize(in->state().name.c_str()).x;
            obj->w_widget_name_max = std::max(obj->w_widget_name_max, in->state().w_name + ImGui::GetStyle().ItemSpacing.x * 2);
            obj->w_widget_value_max = std::max(obj->w_widget_value_max, pin_obj->w_value);
        }
        else
        {
            m_has_in_pins = true;
            in->state().w_name = ImGui::CalcTextSize(in->state().name.c_str()).x;
            obj->w_pin_in_max = std::max(obj->w_pin_in_max, in->state().w_name);
        }
    }

    for (auto& out : m_output)
    {
        m_has_out_pins = true;
        out->state().w_name = ImGui::CalcTextSize(out->state().name.c_str()).x;
        obj->w_pin_out_max = std::max(obj->w_pin_out_max, out->state().w_name);
    }

    // empty
    if (!m_has_in_pins && !m_has_out_pins && !m_has_widgets) [[unlikely]]
    {
        obj->w_fit = m_min_possible_w = 100.0f;
        m_ideal_w = obj->w_fit;
        return obj->w_fit;
    }

    const float min_pins_w = obj->w_pin_in_max + obj->w_pin_out_max + m_min_center_w;
    const float min_widget_w = obj->w_widget_name_max + obj->w_widget_value_max;
    const float pin_l_offset = ro ? w::pin_l_offset_ro : w::pin_l_offset_rw;
    const float max_ico = (use_big_pin_icon ? w::ico_max : w::ico_circle);
    const float pin_off = (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2);
    m_ideal_w = obj->w_fit = std::max(min_pins_w, min_widget_w) + pin_off;

    m_min_possible_w = std::max(m_min_possible_w, (obj->w_widget_name_max + obj->w_widget_value_max) / 3);
    m_min_possible_w = std::max(m_min_possible_w, obj->w_pin_in_max + obj->w_pin_out_max + m_min_center_w);
    m_min_possible_w += pin_off;

    m_saved_width = -1.0f;
    w_node = 0.0f;
    return m_ideal_w;
}

float NodeInstanceBase::recalc_node_width(float new_width, bool ro, bool use_big_pin_icon) noexcept
{
    if (get_object() == nullptr) return new_width;

    if (ro)
        new_width = m_ideal_w;

    if (w_name <= 0.0f)
    {
        w_name = ImGui::CalcTextSize(get_name().c_str()).x;
        w_name += get_additional_name_width();
    }

    if (m_saved_width == new_width) [[likely]]
        return w_node;

    if (!do_need_calc_node_widths()) [[unlikely]]
    {
        w_node = w_name + ImGui::GetStyle().ItemSpacing.x * 2 + get_additional_node_width();
        w_node_name_off = std::max(0.0f, (w_node / 2) - (w_name / 2));
        m_saved_width = new_width;
        return w_node;
    }

    new_width = std::max(m_ideal_w, new_width); // TODO: fix resizing
    new_width = std::max(new_width, m_min_possible_w);
    new_width = std::max(new_width, w_name + ImGui::GetStyle().ItemSpacing.x * 2);

    w_node = new_width;
    w_node_name_off = std::max(0.0f, (new_width / 2) - (w_name / 2));

    const float pin_l_offset = ro ? w::pin_l_offset_ro : w::pin_l_offset_rw;
    const float max_ico = (use_big_pin_icon ? w::ico_max : w::ico_circle);
    const float pin_off = (pin_l_offset * 2) + (max_ico * 2) + (w::pin_r_offset * 2);

    //if (new_width < m_ideal_w) // TODO: fix resizing
    //{
    //    const auto avail_w = new_width - pin_off;
    //    w_widget_name = avail_w / 3;
    //    w_widget_value = avail_w / 3 * 2;
    //    w_pin_in = (avail_w - m_min_center_w) / 2;
    //    w_pin_out = (avail_w - m_min_center_w) / 2;
    //    w_center = m_min_center_w;
    //}
    //else if (new_width > m_ideal_w)
    //{
        w_node = new_width;
        w_widget_name = get_object()->w_widget_name_max;
        w_widget_value = w_node - w_widget_name - pin_off;
        w_pin_in = get_object()->w_pin_in_max;
        w_pin_out = get_object()->w_pin_out_max;
        w_center = w_node - pin_off - w_pin_in - w_pin_out;
    //}
    //else
    //{
    //    w_node = m_ideal_w;
    //    w_widget_name = get_object()->w_widget_name_max;
    //    w_widget_value = get_object()->w_widget_value_max;
    //    w_pin_in = get_object()->w_pin_in_max;
    //    w_pin_out = get_object()->w_pin_out_max;
    //    w_center = w_node - pin_off - w_pin_in - w_pin_out;
    //}

    m_saved_width = new_width;
    return w_node;
}

} // namespace data
