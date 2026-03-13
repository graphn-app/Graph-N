
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

void NodeInstance::init(const blob::Blob& blob)
{
    NodeInstanceBase::init();

    blob::Reader rd(blob);
    deserialize(rd);
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

blob::Writer& NodeInstance::serialize(blob::Writer& blob) const
{
    blob
        .push(m_hid)
        .push(m_iid)
        .push(m_wid)
        .push(m_name)
        .push(m_rc)
        .push(m_clr_node)
        .push(m_clr_group)
        .push(m_clr_head)
        .push(m_clr_group_capt)
        .push(m_group)
        .push(m_show_hidden)
        .push(m_expanded)
        .push(m_pinned)
        .push(cast::to_i32(m_mode))
    ;

    blob.push(cast::to_u64(m_input.size()));
    for (auto in : m_input)
    {
        blob.push(in->get_object()->get_id());
        blob.push(in->get_instance_id());
    }
    blob.push(cast::to_u64(m_output.size()));
    for (auto out : m_output)
    {
        blob.push(out->get_object()->get_id());
        blob.push(out->get_instance_id());
    }

    for (auto in : m_input)
    {
        blob::Block block(blob);
        in->serialize(blob);
    }
    for (auto out : m_output)
    {
        blob::Block block(blob);
        out->serialize(blob);
    }

    return blob;
}

blob::Writer NodeInstance::serialize() const
{
    blob::Writer blob;
    serialize(blob);
    return blob;
}

bool NodeInstance::deserialize(blob::Reader& blob)
{
    try
    {
        m_hid = blob.pop_hid();
        m_iid = blob.pop_iid();
        m_wid = blob.pop_wid();
        m_name = blob.pop_string();
        m_rc = blob.pop_rect();
        m_clr_node = blob.pop_vec4();
        m_clr_group = blob.pop_clr();
        m_clr_head = blob.pop_clr();
        m_clr_group_capt = blob.pop_clr();
        m_group = blob.pop_bool();
        m_show_hidden = blob.pop_bool();
        m_expanded = blob.pop_bool();
        m_pinned = blob.pop_bool();
        m_mode = cast::to_enum<NodeMode>(blob.pop_i32());

        if (!m_group)
        {
            m_object = g::pNodesManager->find_node(m_hid);
            if (nullptr == m_object)
            {
                LogWarn("Node " << m_hid << " not found");
                return false;
            }
        }

        const auto find_pin_in = [](hid pin_id, const std::vector<Pin*>& pins) -> Pin*
        {
            for (auto pin : pins)
            {
                if (pin->get_id() == pin_id)
                    return pin;
            }
            return nullptr;
        };
        const auto find_pin = [node = m_object, &find_pin_in](hid pin_id, bool in)
        {
            if (in)
            {
                auto pin = find_pin_in(pin_id, node->required.pins);
                if (nullptr == pin)
                {
                    pin = find_pin_in(pin_id, node->optional.pins);
                    if (nullptr == pin)
                    {
                        pin = find_pin_in(pin_id, node->hidden.pins);
                    }
                }
                return pin;
            }
            return find_pin_in(pin_id, node->output);
        };

        std::vector<std::pair<Pin*, iid>> in, out;

        auto pins = blob.pop_u64();
        for (uint64_t i = 0; i < pins; i++)
        {
            const auto h_id = blob.pop_hid();
            const auto i_id = blob.pop_iid();
            auto pin = find_pin(h_id, true);
            if (nullptr == pin)
                throw std::runtime_error("Serialized input Pin not found");
            in.push_back(std::make_pair(pin, i_id));
        }
        pins = blob.pop_u64();
        for (uint64_t i = 0; i < pins; i++)
        {
            const auto h_id = blob.pop_hid();
            const auto i_id = blob.pop_iid();
            auto pin = find_pin(h_id, false);
            if (nullptr == pin)
                throw std::runtime_error("Serialized output Pin not found");
            out.push_back(std::make_pair(pin, i_id));
        }

        auto wf = g::pWfsManager->access(m_wid);
        for (auto& pin : in)
        {
            auto instance = wf->add_pin(this, pin.first, pin.second);
            blob::Reader rd_pin = blob.pop_blob();
            instance->deserialize(rd_pin);
            m_input.push_back(instance);
        }
        for (auto& pin : out)
        {
            auto instance = wf->add_pin(this, pin.first, pin.second);
            blob::Reader rd_pin = blob.pop_blob();
            instance->deserialize(rd_pin);
            m_output.push_back(instance);
        }
        return true;
    }
    catch (std::exception& ex)
    {
        LogExcep(ExcepCategory::Logic, ex.what());
        return false;
    }
}

} // namespace data
