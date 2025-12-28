
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
    void set_option(const std::string& key, int64_t value);

    std::optional<std::string> get_option_str(const std::string& key);
    std::optional<int64_t> get_option_int(const std::string& key);

private:
    void set(const std::string& query);

private:
    std::unique_ptr<SQLite::Database> m_db;
};

} // namespace mngr
