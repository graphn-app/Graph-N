
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class DataManager final : public Manager
{
public:
	DataManager() = default;
	~DataManager() override = default;

	bool init() override;
	void free() override;
};

} // namespace mngr