
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class NodesManager final : public Manager
{
public:
	NodesManager() = default;
	~NodesManager() override = default;

	bool init() override;
	void free() override;
};

} // namespace mngr