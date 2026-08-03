
#pragma once
#include "core/data/node.h"

namespace data
{

struct NodeNote final : public Node
{
public:
    using Node::Node;
    ~NodeNote() noexcept = default;
};

} // namespace data
