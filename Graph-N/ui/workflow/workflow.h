
#pragma once

namespace ax::NodeEditor
{
struct EditorContext;
}

namespace data
{
struct Node;
struct Pin;
}

namespace ui
{

class NodeInstance;
class PinInstance;

class Workflow final : public base::LocalID
{
public:
    Workflow();
    ~Workflow() override;

    void init();
    void draw();

    NodeInstance* find_node(aid node_id) const;
    PinInstance* find_pin(aid pin_id) const;

    NodeInstance* add_node(data::Node* node);
    PinInstance* add_pin(data::Pin* pin);

    void remove_invalid(hid id);

private:
    ax::NodeEditor::EditorContext* m_context = nullptr;
    std::map<aid, std::unique_ptr<NodeInstance>> m_nodes;
    std::map<aid, std::unique_ptr<PinInstance>> m_pins;
    std::set<hid> m_invalid_items;
};

} // namespace ui
