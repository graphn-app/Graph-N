
#pragma once

namespace ax::NodeEditor
{
struct EditorContext;
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
    void draw() const;

    NodeInstance* find_node(aid node_id) const;
    PinInstance* find_pin(aid pin_id) const;

    NodeInstance* add_node(hid node_id);
    PinInstance* add_pin(hid pin_id);

    void remove(hid id);

private:
    ax::NodeEditor::EditorContext* m_context = nullptr;
	std::map<aid, std::unique_ptr<NodeInstance>> m_nodes;
    std::map<aid, std::unique_ptr<PinInstance>> m_pins;
};

} // namespace ui