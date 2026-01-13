
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

enum class NodeOption
{
    advanced = 1
};

struct Value final
{
    std::string sval;
    int64_t ival = 0;

    Value(const std::string& s, int64_t i)
        : sval(s)
        , ival(i)
    {}
    Value(int64_t i, const std::string& s)
        : sval(s)
        , ival(i)
    {}
    Value(int64_t i)
        : ival(i)
    {}
    Value(const std::string& s)
        : sval(s)
    {}
};

struct DbOption
{
    std::string name;
    std::optional<Value> val;
};

class DbManager final : public Manager
{
public:
    DbManager() = default;
    ~DbManager() override = default;

    bool init() override;

    void save_exception(ExcepCategory category, const std::string& message, const char* file, int line);

    void save_option(const std::string& key, const Value& value);
    std::vector<DbOption> load_all_options();
    void save_all_options(const std::vector<DbOption>& opts);

    void save_node_option(hid node_hash, NodeOption opt, const Value& value);
    std::optional<Value> load_node_option(hid node_hash, NodeOption opt);
    std::vector<std::pair<hid, Value>> load_all_node_options(NodeOption opt);

private:

private:
    std::unique_ptr<SQLite::Database> m_db;
    std::mutex m_lock;
};

} // namespace mngr
