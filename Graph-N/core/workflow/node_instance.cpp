/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"
#include "imgui_widget_limits.h"

namespace data
{

NodeInstance::~NodeInstance() = default;

void NodeInstance::init(const std::string& select)
{
    NodeInstanceBase::init();

    reset_to_default_colors();

    if (flags().has(NodeInstFlags::Group) || !has_object())
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

    m_input.clear();
    m_output.clear();
    add_pins(get_object()->input, m_input);
    add_pins(get_object()->output, m_output);
}

void NodeInstance::init(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    NodeInstanceBase::init();

    m_rc = copy_from_node->m_rc;
  //  if (copy_from_node->flags().has(NodeInstFlags::Group))
  //      m_rc.Max.y -= ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y * 4;
    m_clr_node = copy_from_node->m_clr_node;
    m_clr_group = copy_from_node->m_clr_group;
    m_clr_head = copy_from_node->m_clr_head;
    m_clr_group_capt = copy_from_node->m_clr_group_capt;
    m_name = copy_from_node->m_name;

    flags().copy_from_by_mask_flags(copy_from_node->flags(),
        NodeInstFlags::Group,
        NodeInstFlags::Collapsed,
        NodeInstFlags::Pinned
    );
    flags().copy_from_by_mask_obj(copy_from_node->flags(), g_node_inst_all_modes_mask);

    if (flags().has(NodeInstFlags::Group))
        return;

    if (cni_copy_from(copy_from_node, copy_pin_ids))
        return;

    auto wf = g::pWfsManager->access(get_workflow_id());

    const auto add_pins = [this, wf, copy_pin_ids](const std::vector<PinInstance*>& source, std::vector<PinInstance*>& dest)
    {
        for (auto pin : source)
        {
            auto pin_instance = wf->add_pin(this, pin->get_object(), (copy_pin_ids ? pin->get_instance_id() : 0));
            auto& state = pin_instance->state();
            pin->state().copy_to(state);
            state.type = pin->get_object()->def_state.type;
            state.type_str = pin->get_object()->def_state.type_str;
            dest.push_back(pin_instance);
        }
    };

    add_pins(copy_from_node->m_input, m_input);
    add_pins(copy_from_node->m_output, m_output);
}

uint64_t NodeInstance::reset_state() noexcept
{
    const auto id = NodeInstanceBase::reset_state();
    flags().remove(NodeInstFlags::PosSet, NodeInstFlags::SizeSet, NodeInstFlags::PinnedSet);
    return id;
}

template <typename T>
T increment_t(T val, const T min, const T max, const T step)
{
    if (val + step > max)
        val = min;
    else
        val = val + step;
    return val;
}

template <typename T>
T decrement_t(T val, const T min, const T max, const T step)
{
    if (val - step < min)
        val = max;
    else
        val = val - step;
    return val;
}

void NodeInstance::increment(const NodeInstance* copy_to, WorkflowAction* wf_action)
{
    if (flags().has(NodeInstFlags::Group) || !has_object() || obj_flags().has(NodeObjFlags::Dummy))
        return;

    auto find_pin = [copy_to](iid pin_id) -> PinInstance*
    {
        if (copy_to != nullptr)
        {
            for (auto pin : copy_to->m_input)
            {
                if (pin->get_instance_id() == pin_id)
                    return pin;
            }
        }
        return nullptr;
    };

    for (auto pin : m_input)
    {
        if (pin->state().cag_val != ControlAfterGenerate::Fixed)
        {
            PinInstance* pin_copy_to = nullptr;
            if (copy_to != nullptr)
                pin_copy_to = find_pin(pin->get_instance_id());

            auto& value = pin->state().value;

            switch (pin->state().cag_val)
            {
            case ControlAfterGenerate::Increment:
                if (value.is_int())
                {
                    *value.get_int_ref() = increment_t(value.get_int(), pin->state().min.get_int(), pin->state().max.get_int(), pin->state().step.get_int());
                }
                else if (value.is_float())
                {
                    *value.get_float_ref() = increment_t(value.get_float(), pin->state().min.get_float(), pin->state().max.get_float(), pin->state().step.get_float());
                }
                else if (value.is_number())
                {
                    *value.get_number_ref() = increment_t(value.get_number(), pin->state().min.get_number(), pin->state().max.get_number(), pin->state().step.get_number());
                }
                break;
            case ControlAfterGenerate::Decrement:
                if (value.is_int())
                {
                    *value.get_int_ref() = decrement_t(value.get_int(), pin->state().min.get_int(), pin->state().max.get_int(), pin->state().step.get_int());
                }
                else if (value.is_float())
                {
                    *value.get_float_ref() = decrement_t(value.get_float(), pin->state().min.get_float(), pin->state().max.get_float(), pin->state().step.get_float());
                }
                else if (value.is_number())
                {
                    *value.get_number_ref() = decrement_t(value.get_number(), pin->state().min.get_number(), pin->state().max.get_number(), pin->state().step.get_number());
                }
                break;
            case ControlAfterGenerate::Randomize:
                if (value.is_int())
                {
                    const int64_t min = pin->state().min.get_int();
                    const int64_t max = pin->state().max.get_int();
                    *value.get_int_ref() = base::GlobalID::random_widget(min, max);
                }
                else if (value.is_float())
                {
                    const int64_t min = cast::to_i64(pin->state().min.get_float());
                    const int64_t max = cast::to_i64(pin->state().max.get_float());
                    *value.get_float_ref() = cast::to_double(base::GlobalID::random_widget(min, max));
                }
                else if (value.is_number())
                {
                    const int64_t min = cast::to_i64(pin->state().min.get_number().underlying_type());
                    const int64_t max = cast::to_i64(pin->state().max.get_number().underlying_type());
                    *value.get_number_ref() = cast::to_number(base::GlobalID::random_widget(min, max));
                }
                break;
            default:
                continue;
            }

            if (pin_copy_to != nullptr)
            {
                auto old_val = pin_copy_to->state().value;
                pin_copy_to->state().value = value;
                wf_action->set_pin_val(pin_copy_to, std::move(old_val));
            }
        }
    }
}

void NodeInstance::collect_search_data(std::stringstream& data) const
{
    if (flags().has(NodeInstFlags::Group) || !has_object() || obj_flags().has(NodeObjFlags::Dummy))
        return;

    for (auto pin : m_input)
    {
        if (PinType::INT == pin->get_object()->def_state.type &&
            tools::str::to_lower_ascii(pin->get_object()->def_state.name).find("seed") != std::string::npos)
        {
            data << pin->state().value.get_int() << " ";
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
        if (str.starts_with("data:image")) return false;

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
                    const std::string& str = pin->state().value.get_string();
                    if (is_valid_search_string(str))
                        data << str << " ";
                }
                if (PinType::COMBO == pin_type)
                {
                    const int64_t index = pin->state().value.get_int();
                    if (index >= 0 && index < cast::to_i64(pin->get_object()->combo_items_ptr.size()))
                    {
                        data << pin->get_object()->combo_items_ptr[index] << " ";
                    }
                }
            }
        }
    }
}

void NodeInstance::copy_attributes(NodeInstance* dest) const noexcept
{
    dest->flags().add_remove_if(NodeInstFlags::Pinned, flags().has(NodeInstFlags::Pinned));
    dest->flags().add_remove_if(NodeInstFlags::Collapsed, flags().has(NodeInstFlags::Collapsed));
    dest->flags().add_remove_if(NodeInstFlags::ModeOnEvent, flags().has(NodeInstFlags::ModeOnEvent));
    dest->flags().add_remove_if(NodeInstFlags::ModeMute, flags().has(NodeInstFlags::ModeMute));
    dest->flags().add_remove_if(NodeInstFlags::ModeOnTrigger, flags().has(NodeInstFlags::ModeOnTrigger));
    dest->flags().add_remove_if(NodeInstFlags::ModeBypass, flags().has(NodeInstFlags::ModeBypass));
    dest->m_clr_head = m_clr_head;
    dest->m_clr_node = m_clr_node;
    dest->m_clr_group = m_clr_group;
    dest->m_clr_group_capt = m_clr_group_capt;
}

const std::string& NodeInstance::get_name() const noexcept
{
    if (flags().has(NodeInstFlags::Group))
        return m_name;
    return NodeInstanceBase::get_name();
}

void NodeInstance::set_name(std::string name) noexcept
{
    if (flags().has(NodeInstFlags::Group))
        m_name = std::move(name);
    else
        NodeInstanceBase::set_name(std::move(name));
}

void NodeInstance::pin_node(bool pin) noexcept
{
    if (flags().has(NodeInstFlags::Pinned) == pin)
        return;
    w_name = -1.0f;
    m_saved_width = -1.0f;
    flags().add_remove_if(NodeInstFlags::Pinned, pin);
    flags().remove(NodeInstFlags::PinnedSet);
}

void NodeInstance::collapse_node(bool collapse) noexcept
{
    if (flags().has(NodeInstFlags::Collapsed) == collapse)
        return;
    flags().add_remove_if(NodeInstFlags::Collapsed, collapse);
    w_name = -1.0f;
    m_saved_width = -1.0f;
}

void NodeInstance::set_mode(NodeInstFlags mode) noexcept
{
    LogRAssert(NodeInstFlags::None == mode || g_node_inst_all_modes_mask.has(mode), "Invalid mode passed", );

    if (mode != NodeInstFlags::None && flags().has(mode))
        return;
    if (NodeInstFlags::ModeBypass == mode && obj_flags().has_not(NodeObjFlags::CanBypass))
        return;
    if (ObjectFlags<NodeInstFlags>::is_one_of(mode, NodeInstFlags::ModeMute, NodeInstFlags::ModeOnEvent, NodeInstFlags::ModeOnTrigger) && obj_flags().has_not(NodeObjFlags::CanMute))
        return;

    w_name = -1.0f;
    m_saved_width = -1.0f;

    flags().remove(
        //NodeInstFlags::ModeAlways, 
        NodeInstFlags::ModeOnEvent, 
        NodeInstFlags::ModeMute, 
        NodeInstFlags::ModeOnTrigger, 
        NodeInstFlags::ModeBypass
    );
    flags().add(mode);
}

ImU32 NodeInstance::get_default_color() noexcept
{
    if (0 == m_def_color)
    {
        ImU32 c2 = 0, c3 = 0, from = 0;
        if (flags().has(NodeInstFlags::Group))
            from = g::pOptsManager->color_load(AppColor::GroupBg);
        else if (obj_flags().has_any(NodeObjFlags::SpecNote, NodeObjFlags::SpecMdNote))
            from = g::pOptsManager->color_load(AppColor::NodeNote);
        else
            from = g::pOptsManager->color_load(AppColor::NodeBodyBg);
        mngr::ModuleColors::make_colors(ImGui::ColorConvertU32ToFloat4(from), m_def_color, c2, c3, flags().has(NodeInstFlags::Group));
    }
    return m_def_color;
}

void NodeInstance::set_group_color(ImU32 clr) noexcept
{
    mngr::ModuleColors::make_colors(ImGui::ColorConvertU32ToFloat4(clr), m_clr_group, m_clr_head, m_clr_group_capt, true);
}

void NodeInstance::set_node_color(ImU32 clr) noexcept
{
    ImU32 dummy = 0;
    mngr::ModuleColors::make_colors(ImGui::ColorConvertU32ToFloat4(clr), m_clr_node, m_clr_head, dummy, false);
}

void NodeInstance::reset_to_default_colors() noexcept
{
    if (flags().has(NodeInstFlags::Group))
    {
        set_group_color(get_default_color());
    }
    else if (has_object())
    {
        set_node_color(get_default_color());
    }
}

void NodeInstance::reset_width() noexcept
{
    if (flags().has_not(NodeInstFlags::Group))
    {
        NodeInstanceBase::reset_width();
        m_rc.Max.x = get_ideal_width();
    }
    flags().remove(NodeInstFlags::SizeSet);
}

float NodeInstance::get_additional_name_width() const noexcept
{
    float add = 0.0f;
    if (flags().has(NodeInstFlags::Pinned))
        add += ImGui::CalcTextSize(ICON_LC_X).x + ImGui::GetStyle().ItemSpacing.x * 2;
    if (!get_mode_always())
        add += ImGui::CalcTextSize(ICON_LC_X).x + ImGui::GetStyle().ItemSpacing.x * 2;
    return add;
}

float NodeInstance::get_additional_node_width() const noexcept
{
    if (flags().has(NodeInstFlags::Collapsed))
    {
        return (m_input.empty() ? 0 : w::pin_l_offset_rw + w::ico_circle + w::pin_r_offset + ImGui::GetStyle().ItemSpacing.x) +
            (m_output.empty() ? 0 : w::pin_l_offset_rw + w::ico_circle + w::pin_r_offset + ImGui::GetStyle().ItemSpacing.x);
    }
    return 0.0f;
}

} // namespace data
