
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class DbManager final : public Manager
{
public:
	DbManager() = default;
	~DbManager() override = default;

	bool init() override;
	void free() override;

	void set_option(const std::string& key, const std::string& value);
	void set_option(const std::string& key, __int32 value);

private:
	std::unique_ptr<SQLite::Database> m_db;
};

} // namespace mngr