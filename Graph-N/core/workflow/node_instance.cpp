
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"

namespace data
{

NodeInstance::~NodeInstance() = default;

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

void NodeInstance::init(const NodeInstance* copy_from_node, bool copy_pin_ids)
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

    const auto add_pins = [this, wf, copy_pin_ids](const std::vector<PinInstance*>& source, std::vector<PinInstance*>& dest)
    {
        for (auto pin : source)
        {
            auto pin_instance = wf->add_pin(this, pin->get_object(), (copy_pin_ids ? pin->get_instance_id() : 0));
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

void NodeInstance::increment(NodeInstance* copy_to, WorkflowAction* wf_action)
{
    if (m_group || nullptr == get_object() || get_object()->is_dummy)
        return;

    auto find_pin = [this](const std::vector<PinInstance*>& input, iid pin_id) -> PinInstance*
    {
        for (auto pin : input)
        {
            if (pin->get_instance_id() == pin_id)
                return pin;
        }
        return nullptr;
    };

    for (auto pin : m_input)
    {
        if (pin->get_object()->cag_require)
        {
            auto& value = pin->state().value;
            if (!std::holds_alternative<uint64_t>(value))
            {
                LogInternalLogic("CAG required pin holds wrong type");
            }
            else
            {
                uint64_t* u64_val = Pin::get_uint(value);

                PinInstance* pin_copy_to = nullptr;
                if (copy_to != nullptr)
                    pin_copy_to = find_pin(copy_to->m_input, pin->get_instance_id());

                auto cag_pin = find_pin(m_input, pin->state().cag);
                if (nullptr == cag_pin)
                {
                    LogInternalLogic("CAG pin not found");
                }
                else if (!cag_pin->get_object()->cag_pin)
                {
                    LogInternalLogic("Linked CAG pin is not CAG");
                }
                else if (cag_pin->get_object()->def_state.type != PinType::COMBO)
                {
                    LogInternalLogic("Linked CAG pin is not COMBO");
                }
                else if (!std::holds_alternative<int64_t>(cag_pin->get_object()->def_state.value))
                {
                    LogInternalLogic("Linked CAG pin holds wrong type");
                }
                else
                {
                    const auto cb_idx = *Pin::get_int(cag_pin->state().value);
                    if (cb_idx < ControlAfterGenerate::_first || cb_idx > ControlAfterGenerate::_last)
                    {
                        LogInternalLogic("Linked CAG pin holds wrong value");
                    }
                    else
                    {
                        Log("Node " << get_name() << ", pin " << pin->state().name << " prev value: " << *u64_val);
                        switch (cb_idx)
                        {
                        case ControlAfterGenerate::CAGFixed:
                            break;
                        case ControlAfterGenerate::CAGDecrement:
                            (*u64_val)--;
                            break;
                        case ControlAfterGenerate::CAGIncrement:
                            (*u64_val)++;
                            break;
                        case ControlAfterGenerate::CAGRandomize:
                            (*u64_val) = base::GlobalID::random();
                            break;
                        default:
                            {
                                LogInternalLogic("Linked CAG pin holds wrong value");
                            }
                        }

                        if (pin_copy_to != nullptr && wf_action != nullptr)
                        {
                            auto old_val = pin_copy_to->state().value;
                            auto& value_to = pin_copy_to->state().value;
                            auto u64_val_copy_to = Pin::get_uint(value_to);
                            *u64_val_copy_to = *u64_val;
                            wf_action->set_pin_val(pin_copy_to, std::move(old_val));
                        }

                        Log("Next value: " << *u64_val);
                    }
                }
            }
        }
    }
}

void NodeInstance::collect_search_data(std::stringstream& data) const
{
    if (m_group || nullptr == get_object() || get_object()->is_dummy)
        return;

    for (auto pin : m_input)
    {
        if ((PinType::INT == pin->get_object()->def_state.type || PinType::NUMBER == pin->get_object()->def_state.type) &&
            (tools::str::to_lower_ascii(pin->get_object()->def_state.name).find("seed") != std::string::npos))
        {
            data << *Pin::get_uint(pin->state().value) << " ";
            break;
        }
    }

    const auto name = tools::str::to_lower_ascii(get_name_original());

    const std::pair<const char*, PinType> nodes[] =
    {
        { "clip", PinType::STRING },     // prompt texts
        { "load", PinType::COMBO },      // LoRA, checkpoint, model, IPAdapter etc.
        { "note", PinType::STRING },     // note, markdown note
        { "sampler", PinType::COMBO },   // sampler names (euler, ddim) and schedulers (karras, normal)
        { "text", PinType::STRING },     // custom text nodes, primitive text nodes
        { "string", PinType::STRING },   // alternative naming for custom text blocks
        { "save", PinType::STRING }      // filename_prefix in SaveImage and similar nodes
    };

    const auto is_valid_search_string = [](const std::string& str)
    {
        constexpr size_t max_text_len = 16384;
        constexpr size_t max_word_len = 257;

        if (str.empty() || str.size() > max_text_len) return false;
        if (str.find("data:image") == 0) return false;

        if (str.size() > max_word_len)
        {
            size_t current_len = 0;
            for (const char c : str)
            {
                if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == ',' || c == '.')
                {
                    current_len = 0;
                }
                else
                {
                    if (++current_len > max_word_len) 
                        return false;
                }
            }
        }
        return true;
    };


    for (auto& [node_sub, pin_type] : nodes)
    {
        if (name.find(node_sub) == std::string::npos)
            continue;

        for (auto pin : m_input)
        {
            if (pin_type == pin->get_object()->def_state.type)
            {
                if (PinType::STRING == pin_type)
                {
                    const std::string* str = Pin::get_string(pin->state().value);
                    if (is_valid_search_string(*str))
                        data << *str << " ";
                }
                if (PinType::COMBO == pin_type)
                {
                    const int64_t* index = data::Pin::get_int(pin->state().value);
                    if (*index >= 0 && *index < cast::to_i64(pin->get_object()->combo_items_ptr.size()))
                    {
                        data << pin->get_object()->combo_items_ptr[*index] << " ";
                    }
                }
            }
        }
    }
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

void NodeInstance::expand(bool expand)
{
    if (m_expanded == expand)
        return;
    m_expanded = expand;
    w_name = -1.0f;
    m_saved_width = -1.0f;
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

float NodeInstance::get_additional_node_width() const
{
    if (!m_expanded)
    {
        return (m_input.empty() ? 0 : w::pin_l_offset_rw + w::ico_circle + w::pin_r_offset + ImGui::GetStyle().ItemSpacing.x) +
            (m_output.empty() ? 0 : w::pin_l_offset_rw + w::ico_circle + w::pin_r_offset + ImGui::GetStyle().ItemSpacing.x);
    }
    return 0.0f;
}

bool NodeInstance::do_need_calc_node_widths() const
{
    return (m_expanded && !m_group);
}

} // namespace data
