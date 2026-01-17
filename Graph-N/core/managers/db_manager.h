
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class DbManager final : public Manager
{
public:
    DbManager() = default;
    ~DbManager() override;

    bool init() override;
    inline bool get_is_first_run() const { return m_first_run; }


    void exception_save(ExcepCategory category, const std::string& message, const char* file, int line);

    void option_save(const std::string& key, const DbValue& value);
    std::vector<DbOption> option_load_all();
    void option_save_all(const std::vector<DbOption>& opts);

    void node_option_save(hid node_hash, NodeOption opt, const DbValue& value);
    std::optional<DbValue> node_option_load(hid node_hash, NodeOption opt);
    std::vector<std::tuple<hid, NodeOption, DbValue>> node_option_load_all();

    void wf_delete(const WfData& wf);
    std::vector<WfData> wf_load_meta_delete_empty();
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
