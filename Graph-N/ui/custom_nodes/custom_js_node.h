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
    void draw(data::NodeInstance* node);

private:
    void draw_LoadImageFromUrl(data::NodeInstance* node, js::Js_LoadImageFromUrl* data);
};

} // namespace ui
