
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
    ~DbManager() override;

    bool init() override;
    inline bool get_is_first_run() const { return m_first_run; }


    void exception_save(ExcepCategory category, const std::string& message, const char* file, int line);

    void option_save(const std::string& key, const Value& value);
    std::vector<DbOption> option_load_all();
    void option_save_all(const std::vector<DbOption>& opts);

    void node_option_save(hid node_hash, NodeOption opt, const Value& value);
    std::optional<Value> node_option_load(hid node_hash, NodeOption opt);
    std::vector<std::pair<hid, Value>> node_option_load_all(NodeOption opt);

    void wf_delete(const WfData& wf);
    std::vector<WfData> wf_load_meta();
    void wf_update_meta(const WfData& wf);
    void wf_load_json(WfData& wf);
    void wf_update_json(const WfData& wf);    

private:

private:
    std::unique_ptr<SQLite::Database> m_db;
    std::mutex m_lock;
    bool m_first_run = true;
};

} // namespace mngr
