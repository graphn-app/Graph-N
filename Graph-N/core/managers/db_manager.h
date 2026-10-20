/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/managers/manager.h"

namespace mngr
{

struct DbMetadata final
{
    app_version_type version = 0;
    std::string last_run;
    std::string customer_id;
    int64_t last_session_id = 0;
    std::string last_session_duration;
};

class DbManager final : public Manager
{
public:
    DbManager();
    ~DbManager() override;

    const DbMetadata& get_prev_db_metadata() const noexcept { return m_prev_meta; }
    static DbMetadata read_db_metadata();

    bool init() override;
    void stop_and_cleanup() override;

    bool get_is_first_run() const noexcept { return m_first_run; }
    void begin_transaction();
    void end_transaction();

    void exception_save(std::string &&message, const char *file, int32_t line) noexcept;
    void session_end_save(int64_t session_id, const std::string& duration);

    void session_nodes_load(std::unordered_set<std::string>& resolved_nodes, std::unordered_set<std::string>& unresolved_nodes, std::unordered_set<std::string>& python_modules);
    void session_nodes_save(const std::unordered_set<std::string>& resolved_nodes, const std::unordered_set<std::string>& unresolved_nodes, const std::unordered_set<std::string>& python_modules);

    void app_option_save(const std::string& key, DbValue&& value);
    std::vector<DbAppOptionData> app_option_load_all();
    void app_option_save_all(std::vector<DbAppOptionData>&& opts);

    void item_option_save(DbItemOptionType type, hid hash, DbValue&& value);
    std::optional<DbValue> item_option_load(DbItemOptionType type, hid hash);
    std::vector<std::pair<hid, DbValue>> item_option_load_all(DbItemOptionType type);

    std::vector<WfData> wf_load_meta_and_purge();
    void wf_update_meta(const WfData& wf);
    void wf_load_blob(const std::string& uuid, blob::Blob& blob, bool& load_saved);
    void wf_update_blob(const std::string& uuid, blob::Blob&& blob, bool draft, tools::dt time = {});

    void history_add(ExecTask& task);
    std::vector<ExecTask> history_load_all();
    void history_delete(const std::string& prompt_id);
    void history_search(const std::string& prompt, std::list<std::string>& wfs);

private:
    enum class WriteType : uint8_t
    {
        AppOption, Item, WfMeta, WfBlob, WfPurge, HistoryAdd, HistoryDel, SessionEnd, NodesData
    };
    struct ExcepData final
    {
        std::string message;
        std::string file;
        int32_t line = 0;
    };
    struct ItemData final
    {
        hid hash = 0;
        DbItemOptionType type;
        DbValue value;
    };
    struct WfBlob final
    {
        std::string uuid;
        blob::Blob blob;
        bool draft;
        tools::dt time;
    };
    struct HistoryData final
    {
        blob::Blob blob;
        std::string prompt_id;
        std::string flow_data;
        std::string search_data;
    };
    struct SessionData final
    {
        int64_t session_id = 0;
        std::string session_duration;
    };
    struct NodesData final
    {
        std::string resolved_nodes;
        std::string unresolved_nodes;
        std::string python_modules;
    };
    struct WriteData final
    {
        WriteType type;
        std::variant<DbAppOptionData, ItemData, WfData, WfBlob, HistoryData, SessionData, NodesData> data;
    };

private:
    void backup_thread(std::filesystem::path db_path, std::unique_ptr<SQLite::Database> db);
    void write_thread(std::filesystem::path db_path);
    void write_data(const SQLite::Database& db, const ExcepData& data);
    void write_data(const SQLite::Database& db, const DbAppOptionData* data);
    void write_data(const SQLite::Database& db, const ItemData* data);
    void delete_data(const SQLite::Database& db, const ItemData* data);
    void write_wf_meta(const SQLite::Database& db, const WfData* data);
    void write_wf_blob(const SQLite::Database& db, const WfBlob* data);
    void delete_wf_draft(const SQLite::Database& db, const WfBlob* data);
    void purge_wf(const SQLite::Database& db);
    void add_history(const SQLite::Database& db, const HistoryData* data);
    void del_history(const SQLite::Database& db, const HistoryData* data);
    void write_session_end(const SQLite::Database& db, const SessionData* data);
    void write_nodes(const SQLite::Database& db, const NodesData* data);

private:
    std::thread m_th_write;
    std::thread m_th_backup;
    std::mutex m_lock;
    std::condition_variable m_cv;
    std::queue<WriteData> m_write_queue;
    std::stop_source m_stop_flag;

    ExcepData m_exception_write_data;
    std::atomic_bool m_exception_write_data_flag = false;

    std::list<WriteData> m_transaction_queue;
    std::atomic_bool m_transaction_flag = false;

    std::unique_ptr<SQLite::Database> m_db;
    bool m_first_run = true;
    DbMetadata m_prev_meta;
};

} // namespace mngr
