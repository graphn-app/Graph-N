
#pragma once
#include "core/workflow/node_instance_base.h"

namespace data
{

class PinInstance;
struct Node;

class NodeInstance final : public NodeInstanceBase
{
public:
    using NodeInstanceBase::NodeInstanceBase;
    ~NodeInstance() override;

    void init(const std::string& select);

    const std::string& get_name() const override;
    void set_name(std::string name) override;

    void toggle_pinned();
    inline bool pinned() const { return m_pinned; }

    void set_mode(NodeMode mode);
    inline NodeMode get_mode() const { return m_mode; }

    void toggle_expanded();
    inline bool expanded() const { return m_expanded; }

    void set_group_color(ImU32 clr);
    void reset_to_default_colors();

    void reset_width();

protected:
    float get_additional_name_width() const override;
    bool do_need_calc_node_widths() const override;

public:
    bool m_pos_set = false;
    ImVec2 m_pos = {};

    bool m_size_set = false;
    ImVec2 m_size = {}; // size for group, width for node

    bool m_bg_color_custom = false;
    ImVec4 m_bg_color = {};
    ImU32 m_header_color = 0;
    ImU32 m_group_caption_color = 0;

    bool m_group = false;
    bool m_show_hidden = false;

private:
    bool m_expanded = true;
    NodeMode m_mode = NodeMode::Always;
    bool m_pinned = false;
};

} // namespace data

