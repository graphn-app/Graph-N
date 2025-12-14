
#pragma once
#include "core/data/node.h"

namespace data
{

struct NodeMarkdownNote final : public Node
{
	NodeMarkdownNote() = default;
	~NodeMarkdownNote() override = default;
};

} // namespace data