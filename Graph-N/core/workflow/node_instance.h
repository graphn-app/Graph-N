/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/workflow/node_instance_ro.h"

namespace data
{

class PinInstance;
struct Node;
class WorkflowAction;

class NodeInstance : public NodeInstanceRo
{
public:
    using NodeInstanceRo::NodeInstanceRo;
    ~NodeInstance() noexcept override;

    virtual void init(const std::string& select);
    virtual void init(const NodeInstance* copy_from_node, bool copy_pin_ids);
    void init_blank() { NodeInstanceBase::init(); }

    const std::string& get_name() const noexcept override;
    void set_name(std::string name) noexcept override;

    void pin_node(bool pin) noexcept;
    void collapse_node(bool collapse) noexcept;

    void set_mode(NodeInstFlags mode) noexcept;
    bool get_mode_always() const noexcept { return flags().has_none(NodeInstFlags::ModeBypass, NodeInstFlags::ModeMute, NodeInstFlags::ModeOnEvent, NodeInstFlags::ModeOnTrigger); }
    bool get_mode_bypass() const noexcept { return flags().has(NodeInstFlags::ModeBypass); }
    bool get_mode_mute() const noexcept { return flags().has_any(NodeInstFlags::ModeMute, NodeInstFlags::ModeOnEvent, NodeInstFlags::ModeOnTrigger); }

    ImU32 get_default_color() noexcept;
    void set_group_color(ImU32 clr) noexcept;
    void set_node_color(ImU32 clr) noexcept;
    void reset_to_default_colors() noexcept;

    void reset_width() noexcept override;
    uint64_t reset_state() noexcept override;

    void increment(const NodeInstance* copy_to, WorkflowAction* wf_action);
    void collect_search_data(std::stringstream& data) const;

    void copy_attributes(NodeInstance* dest) const noexcept;

protected:
    float get_additional_name_width() const noexcept override;
    float get_additional_node_width() const noexcept override;
    bool do_need_calc_node_widths() const noexcept override { return flags().has_none(NodeInstFlags::Collapsed, NodeInstFlags::Group); }

public:
    ImRect m_rc = {};               // Min.x/y: pos for all; Max.x/y: size for group, width for node
    ImU32 m_def_color = 0;
    ImU32 m_clr_node = 0;           // color for node backgroud
    ImU32 m_clr_group = 0;          // color for group backgroud
    ImU32 m_clr_head = 0;           // color for node/group header background
    ImU32 m_clr_group_capt = 0;     // color for group name

    int32_t m_media_list_version = 0;

    float m_last_pins_h = 0.0f;
    float m_last_custom_widgets_h = 0.0f;
    float m_last_all_widgets_h = 0.0f;

    std::unique_ptr<js::Data> m_js_data;
};

} // namespace data
