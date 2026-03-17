
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"

namespace data
{

NodeInstance::~NodeInstance() {};

void NodeInstance::init(const std::string& select)
{
    NodeInstanceBase::init();

    reset_to_default_colors();

    if (m_group)
        return;

    auto wf = g::pWfsManager->access(m_wid);

    const auto add_pins = [this, wf, &select](const std::vector<Pin*>& source, std::vector<PinInstance*>& dest)
    {
        for (auto pin : source)
        {
            auto pin_instance = wf->add_pin(this, pin);
            if (!select.empty())
            {
                const auto& ptrs = pin_instance->get_object()->combo_items_ptr;
                for (int32_t i = 0; i < ptrs.size(); i++)
                {
                    if (ptrs[i] == select)
                    {
                        pin_instance->state().value = int64_t(i);
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

    link_cag_pins();
}

void NodeInstance::init(const NodeInstance* copy_from_node)
{
    NodeInstanceBase::init();

    m_rc = copy_from_node->m_rc;
  //  if (copy_from_node->m_group)
  //      m_rc.Max.y -= ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y * 4;
    m_clr_node = copy_from_node->m_clr_node;
    m_clr_group = copy_from_node->m_clr_group;
    m_clr_head = copy_from_node->m_clr_head;
    m_clr_group_capt = copy_from_node->m_clr_group_capt;
    m_group = copy_from_node->m_group;
    m_expanded = copy_from_node->m_expanded;
    m_mode = copy_from_node->m_mode;
    m_pinned = copy_from_node->m_pinned;
    m_name = copy_from_node->m_name;

    if (m_group)
        return;

    auto wf = g::pWfsManager->access(m_wid);

    const auto add_pins = [this, wf](const std::vector<PinInstance*>& source, std::vector<PinInstance*>& dest)
    {
        for (auto pin : source)
        {
            auto pin_instance = wf->add_pin(this, pin->get_object());
            auto& state = pin_instance->state();
            state = pin->state();
            state.links = 0;
            state.converted = false;
            state.type = pin->get_object()->def_state.type;
            state.type_str = pin->get_object()->def_state.type_str;
            dest.push_back(pin_instance);
        }
    };

    add_pins(copy_from_node->m_input, m_input);
    add_pins(copy_from_node->m_output, m_output);

    link_cag_pins();
}

void NodeInstance::reset_state()
{
    NodeInstanceBase::reset_state();
    m_pos_set = m_size_set = m_pinned_set = false;
}

const std::string& NodeInstance::get_name() const
{
    if (m_group)
        return m_name;
    return NodeInstanceBase::get_name();
}

void NodeInstance::set_name(std::string name)
{
    if (m_group)
        m_name = std::move(name);
    else
        NodeInstanceBase::set_name(std::move(name));
}

void NodeInstance::toggle_pinned()
{
    w_name = -1.0f;
    m_saved_width = -1.0f;
    m_pinned = !m_pinned;
}

void NodeInstance::pin(bool pin)
{
    w_name = -1.0f;
    m_saved_width = -1.0f;
    m_pinned = pin;
    m_pinned_set = false;
}

void NodeInstance::set_mode(NodeMode mode)
{
    w_name = -1.0f;
    m_saved_width = -1.0f;
    m_mode = mode;
}

void NodeInstance::toggle_expanded()
{
    w_name = -1.0f;
    m_saved_width = -1.0f;
    m_expanded = !m_expanded;
}

void NodeInstance::set_group_color(ImVec4 clr)
{
    clr.w = 0.3f;
    m_clr_group = ImGui::ColorConvertFloat4ToU32(clr);

    clr.w = 0.7f;
    m_clr_head = ImGui::ColorConvertFloat4ToU32(clr);

    clr.w = 0.9f;
    m_clr_group_capt = ImGui::ColorConvertFloat4ToU32(clr);
    m_clr_group_capt = ImGuiEx::increase_brightness(m_clr_group_capt);
}

void NodeInstance::set_node_color(const ImVec4& clr)
{
    m_clr_node = clr;
    m_clr_head = ImGuiEx::decrease_brightness(ImGui::ColorConvertFloat4ToU32(m_clr_node));
}

void NodeInstance::reset_to_default_colors()
{
    if (m_group)
    {
        set_group_color(ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::GroupBg)));
    }
    else
    {
        set_node_color(ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::NodeBodyBg)));
    }
}

void NodeInstance::reset_width()
{
    calc_node_width(false, false);
    m_rc.Max.x = get_ideal_width();
    m_size_set = false;
    w_name = -1.0f;
    m_saved_width = -1.0f;
    w_node = 0.0f;
}

float NodeInstance::get_additional_name_width() const
{
    float add = 0.0f;
    if (m_pinned)
        add += ImGui::CalcTextSize(ICON_LC_X).x + ImGui::GetStyle().ItemSpacing.x * 2;
    if (m_mode != NodeMode::Always)
        add += ImGui::CalcTextSize(ICON_LC_X).x + ImGui::GetStyle().ItemSpacing.x * 2;
    return add;
}

bool NodeInstance::do_need_calc_node_widths() const
{
    return (m_expanded && !m_group);
}

} // namespace data
