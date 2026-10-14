#pragma once

namespace data
{
struct Node;
class NodeInstance;
}

namespace ui
{

class CustomJsNode final
{
public:
    NO_COPY_MOVE_CTORS(CustomJsNode);

    void test(data::Node* node);
    void init(data::NodeInstance* node);
    void draw(data::NodeInstance* node, float off, float width);

private:
    void draw_FastActionsButtonRgthree(data::NodeInstance* node, float off, float width);
    void draw_CustomCombo(data::NodeInstance* node, float off, float width);
};

} // namespace ui
