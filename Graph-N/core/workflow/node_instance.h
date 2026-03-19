
#pragma once
#include "core/workflow/node_instance_base.h"

namespace data
{

class PinInstance;
struct Node;

class NodeInstance final : public NodeInstanceBase
{
    friend class WorkflowSerializerToBlob;

public:
    using NodeInstanceBase::NodeInstanceBase;
    ~NodeInstance() override;

    void init(const std::string& select);
    void init(const NodeInstance* copy_from_node);
    inline void init_blank() { NodeInstanceBase::init(); }

    const std::string& get_name() const override;
    void set_name(std::string name) override;

    void pin(bool pin);
    inline bool pinned() const { return m_pinned; }

    void set_mode(NodeMode mode);
    inline NodeMode get_mode() const { return m_mode; }

    void expand(bool expand);
    inline bool get_expanded() const { return m_expanded; }

    void set_group_color(ImVec4 clr);
    void set_node_color(const ImVec4& clr);
    void reset_to_default_colors();

    void reset_width();
    void reset_state() override;

protected:
    float get_additional_name_width() const override;
    float get_additional_node_width() const override;
    bool do_need_calc_node_widths() const override;
    bool do_invalidation() const override { return !m_group; }

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

private:
    NodeMode m_mode = NodeMode::Always;
    bool m_pinned = false;
    bool m_expanded = true;

    // !!
    // check `init`, `reset_state` and `[de]serialize` when adding new field
};

} // namespace data

