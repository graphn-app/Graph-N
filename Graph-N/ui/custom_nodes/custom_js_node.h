/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace data
{
struct Node;
class NodeInstance;
}
namespace mngr
{
class NodesManager;
}

namespace ui
{

class CustomJsNode final
{
public:
    NO_COPY_MOVE_CTORS(CustomJsNode);

    void test(mngr::NodesManager* manager, data::Node* node);
    void init(data::NodeInstance* node);
    void draw(data::NodeInstance* node, float off, float width);

private:
    void draw_FastActionsButtonRgthree(data::NodeInstance* node, float off, float width);
    void draw_CustomCombo(data::NodeInstance* node, float off, float width);
};

} // namespace ui
