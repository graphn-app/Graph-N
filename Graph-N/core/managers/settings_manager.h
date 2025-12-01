
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class SettingsManager final : public Manager
{
public:
	SettingsManager() = default;
	~SettingsManager() override = default;

	bool init() override;
	void free() override;
};

} // namespace mngr