
#pragma once
#include "core/data/node.h"

namespace data
{

struct NodeNote final : public Node
{
    using Node::Node;
	~NodeNote() override = default;
};

} // namespace data