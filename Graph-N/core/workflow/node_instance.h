
#pragma once
#include "core/workflow/instance.h"

namespace data
{

class PinInstance;
struct Node;

class NodeInstance final : public Instance<Node>
{
public:
    using Instance<data::Node>::Instance;
    ~NodeInstance() override;

    void init(const std::string& select);

    float calc_node_width(bool ro);
    float recalc_node_width(bool ro, float avail_width, bool allow_extend, bool allow_compress, bool use_big_pin_icon);

    const std::string& get_name() const;
    void set_name(std::string name);

    void toggle_pinned();
    inline bool pinned() const { return m_pinned; }

    void set_mode(NodeMode mode);
    inline NodeMode get_mode() const { return m_mode; }

    void toggle_expanded();
    inline bool expanded() const { return m_expanded; }

    void invalidate_links();

    void set_group_color(ImU32 clr);
    void reset_to_default_colors();

protected:
    Node* find_obj(hid obj_id) const override;

public:
    std::vector<PinInstance*> m_input;
    std::vector<PinInstance*> m_output;

    float w_node = 0.0f;
    float w_node_name_off = 0.0f;
    float w_name = -1.0f;
    float w_widget_name = 0.0f;
    float w_widget_value = 0.0f;
    float w_center = 0.0f;
    float w_pin_in = 0.0f;
    float w_pin_out = 0.0f;
    float w_separator = 0.0f;

    bool m_pos_set = false;
    ImVec2 m_pos = {};

    bool m_size_set = false;
    ImVec2 m_size = {}; // size for group, width for node

    bool m_bg_color_custom = false;
    ImVec4 m_bg_color = {};
    ImU32 m_header_color = 0;
    ImU32 m_group_caption_color = 0;

    bool m_has_in_pins = false;
    bool m_has_in_pins_linked = false;
    bool m_has_out_pins = false;
    bool m_has_out_pins_linked = false;
    bool m_has_widgets = false;
    bool m_show_hidden = false;

    bool m_group = false;

private:
    bool m_expanded = true;
    NodeMode m_mode = NodeMode::Always;
    bool m_pinned = false;
    float m_saved_width = -1.0f;
    std::string m_name;
};

} // namespace data

