
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

    void pin(bool pin) noexcept;
    bool get_pinned() const noexcept { return m_pinned; }

    void set_mode(NodeMode mode) noexcept;
    NodeMode get_mode() const noexcept { return m_mode; }

    void expand(bool expand) noexcept;
    bool get_expanded() const noexcept { return m_expanded; }

    void set_group_color(ImVec4 clr) noexcept;
    void set_node_color(const ImVec4& clr) noexcept;
    void reset_to_default_colors() noexcept;

    void reset_width() noexcept override;
    void reset_state() noexcept override;

    void increment(const NodeInstance* copy_to, WorkflowAction* wf_action);
    void collect_search_data(std::stringstream& data) const;

protected:
    float get_additional_name_width() const noexcept override;
    float get_additional_node_width() const noexcept override;
    bool do_need_calc_node_widths() const noexcept override { return (m_expanded && !m_group); }

public:
    bool m_pos_set = false;
    bool m_size_set = false;
    bool m_ignore_size_on_frame_end = false;
    bool m_pinned_set = false;

    ImRect m_rc = {}; // size for group, width for node

    ImVec4 m_clr_node = {}; // color for node backgroud
    ImU32 m_clr_group = 0; // color for group backgroud
    ImU32 m_clr_head = 0; // color for node/group header background
    ImU32 m_clr_group_capt = 0; // color for group name

    bool m_group = false;
    bool m_show_hidden = false;

protected:
    bool m_deserialization_mode = false;

private:
    NodeMode m_mode = NodeMode::Always;
    bool m_pinned = false;
    bool m_expanded = true;

    // !!
    // check `init`, `reset_state` and `[de]serialize` when adding new field
};

} // namespace data
