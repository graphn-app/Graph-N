#pragma once

namespace ui
{
class NodeDrawer;
};

namespace data
{

class Link;
struct Node;
class NodeInstance;
class PinInstance;
class Workflow;

class WorkflowAction final
{
public:
    WorkflowAction(data::Workflow* wf);
    ~WorkflowAction();

    inline void set_drawer(ui::NodeDrawer* drawer) { m_drawer = drawer; }
    const blob::Blob* get_drawer_state() const;

    void multi_action_on(const char* action_name) const;
    void multi_action_off() const;

    data::NodeInstance* add_node(data::Node* node, const std::string& select, std::optional<ImRect> pos);
    data::NodeInstance* add_group(std::optional<ImRect> size);

    void delete_node(data::NodeInstance* node);
    void delete_group(data::NodeInstance* group);

    void hard_reset();
    void reinit(blob::Blob&& state);

    data::Link* link(data::PinInstance* p1, data::PinInstance* p2, iid instance_id = 0);
    void unlink(data::PinInstance* pin);
    void unlink(data::PinInstance* p1, data::PinInstance* p2);
    void unlink(data::Link* link);

    void set_node_color(data::NodeInstance* node, ImU32 color);
    void reset_node_color(data::NodeInstance* node);

    void set_node_name(data::NodeInstance* node, const std::string& name);
    void reset_node_name(data::NodeInstance* node);

    bool set_group_rect(data::NodeInstance* group, const ImRect& rect);
    void reset_node_size(data::NodeInstance* node);

    void set_node_expanded(data::NodeInstance* node, bool expanded);
    void set_node_pinning(data::NodeInstance* node, bool pin);
    void set_node_mode(data::NodeInstance* node, NodeMode mode);

    void begin_nodes_move(const std::set<data::NodeInstance*>& nodes);
    void end_nodes_move(bool cancel);
    inline bool is_moving() const { return m_moving; }

    void set_pin_name(data::PinInstance* pin, const std::string& name);
    void reset_pin_name(data::PinInstance* pin);

    void set_pin_val(data::PinInstance* pin, primitive&& old_val);
    void set_pin_val(data::PinInstance* pin, int64_t old_idx);
    void set_pin_val(data::PinInstance* pin, const float* new_colors);

private:
    data::Workflow* m_wf = nullptr;
    ui::NodeDrawer* m_drawer = nullptr;

    bool m_moving = false;
    std::map<data::NodeInstance*, ImRect> m_moving_nodes;
};

} // namespace data

