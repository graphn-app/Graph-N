
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class NodesManager final : public Manager
{
public:
	NodesManager() = default;
	~NodesManager() override = default;

	void free() override;

	void add_node(const std::string& node_name, const std::string& node_info);
};

} // namespace mngr