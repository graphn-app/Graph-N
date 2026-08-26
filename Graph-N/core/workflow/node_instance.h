
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
    void expand_node(bool expand) noexcept;

    void set_mode(NodeInstFlags mode) noexcept;
    bool get_mode_always() const noexcept { return flags().has(NodeInstFlags::ModeAlways); }
    bool get_mode_bypass() const noexcept { return flags().has(NodeInstFlags::ModeBypass); }
    bool get_mode_mute() const noexcept { return flags().has_any(NodeInstFlags::ModeMute, NodeInstFlags::ModeOnEvent, NodeInstFlags::ModeOnTrigger); }

    void set_group_color(ImVec4 clr) noexcept;
    void set_node_color(const ImVec4& clr) noexcept;
    void reset_to_default_colors() noexcept;

    void reset_width() noexcept override;
    uint64_t reset_state() noexcept override;

    void increment(const NodeInstance* copy_to, WorkflowAction* wf_action);
    void collect_search_data(std::stringstream& data) const;

protected:
    float get_additional_name_width() const noexcept override;
    float get_additional_node_width() const noexcept override;
    bool do_need_calc_node_widths() const noexcept override { return flags().has_and_has_not(NodeInstFlags::Expanded, NodeInstFlags::Group); }

public:
    ImRect m_rc = {}; // size for group, width for node
    ImVec4 m_clr_node = {}; // color for node backgroud
    ImU32 m_clr_group = 0; // color for group backgroud
    ImU32 m_clr_head = 0; // color for node/group header background
    ImU32 m_clr_group_capt = 0; // color for group name
};

} // namespace data
