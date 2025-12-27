
#pragma once
#include "core/data/node.h"

namespace data
{

struct NodeMarkdownNote final : public Node
{
    using Node::Node;
	~NodeMarkdownNote() override = default;
};

} // namespace data