
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
    void stop_all_threads() override;

    inline bool get_is_first_run() const { return m_first_run; }


    void exception_save(ExcepCategory category, std::string&& message, const char* file, int32_t line);

    void option_save(const std::string& key, DbValue&& value);
    std::vector<DbOption> option_load_all();
    void option_save_all(std::vector<DbOption>&& opts);

    void node_option_save(hid node_hash, NodeOption opt, DbValue&& value);
    std::optional<DbValue> node_option_load(hid node_hash, NodeOption opt);
    std::vector<std::tuple<hid, NodeOption, DbValue>> node_option_load_all();

    std::vector<WfData> wf_load_meta_and_purge();
    void wf_load_json(WfData& wf);
    void wf_update_meta(const WfData& wf);
    void wf_update_json(WfData& wf);    // wf.json will be moved out

private:
    enum class WriteType
    {
        Exception, Option, Node, WfMeta, WfJson, WfPurge
    };
    struct ExcepData final
    {
        ExcepCategory category;
        std::string message;
        std::string file;
        int32_t line = 0;
    };
    struct NodeData final
    {
        hid node_hash = 0;
        NodeOption opt;
        DbValue value;
    };
    struct WriteData final
    {
        WriteType type;
        std::variant<ExcepData, DbOption, NodeData, WfData> data;
    };

private:
    void write_thread(std::filesystem::path db_path);
    void write_data(SQLite::Database& db, const ExcepData* data);
    void write_data(SQLite::Database& db, const DbOption* data);
    void write_data(SQLite::Database& db, const NodeData* data);
    void delete_data(SQLite::Database& db, const NodeData* data);
    void write_wf_meta(SQLite::Database& db, const WfData* data);
    void write_wf_json(SQLite::Database& db, const WfData* data);
    void delete_wf_json(SQLite::Database& db, const WfData* data);
    void purge_wf(SQLite::Database& db);

private:
    std::thread m_th;
    std::mutex m_lock;
    std::condition_variable m_cv;
    std::atomic_bool m_stop_thread_flag = false;
    std::queue<WriteData> m_write_queue;

    std::unique_ptr<SQLite::Database> m_db;
    bool m_first_run = true;
};

} // namespace mngr
