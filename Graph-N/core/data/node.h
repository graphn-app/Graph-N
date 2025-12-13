
#pragma once
#include "core/base/id.h"

namespace data
{

class Node final : public base::LocalID
{
public:
	Node() = default;
	~Node() override = default;
};

} // namespace data