
#pragma once
#include "core/data/node.h"

namespace data
{

struct NodeMarkdownNote final : public Node
{
public:
    using Node::Node;
    ~NodeMarkdownNote() = default;
};

} // namespace data