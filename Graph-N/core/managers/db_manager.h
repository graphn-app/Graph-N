
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

struct DbMetadata
{
    uint32_t db_ver;
    std::string app_ver;
    std::string last_run;
};

class DbManager final : public Manager
{
public:
    DbManager() = default;
    ~DbManager() override;

    static DbMetadata read_db_metadata();

    bool init() override;
    void stop_all_threads() override;

    inline bool get_is_first_run() const { return m_first_run; }
    void begin_transaction();
    void end_transaction();

    void exception_save(ExcepCategory category, std::string&& message, const char* file, int32_t line);

    void app_option_save(const std::string& key, DbValue&& value);
    std::vector<DbAppOptionData> app_option_load_all();
    void app_option_save_all(std::vector<DbAppOptionData>&& opts);

    void item_option_save(hid id, DbItemOptionType type, DbValue&& value);
    std::optional<DbValue> item_option_load(hid id, DbItemOptionType type);
    std::vector<std::pair<hid, DbValue>> item_option_load_all(DbItemOptionType type);

    std::vector<WfData> wf_load_meta_and_purge();
    void wf_update_meta(const WfData& wf);
    void wf_load_blob(const std::string& uuid, blob::Blob& blob, bool& load_saved);
    void wf_update_blob(const std::string& uuid, blob::Blob&& blob, bool draft);

private:
    enum class WriteType
    {
        Exception, AppOption, Item, WfMeta, WfBlob, WfPurge
    };
    struct ExcepData final
    {
        ExcepCategory category;
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
    struct WfBlob
    {
        std::string uuid;
        blob::Blob blob;
        bool draft;
    };
    struct WriteData final
    {
        WriteType type;
        std::variant<ExcepData, DbAppOptionData, ItemData, WfData, WfBlob> data;
    };

private:
    void backup_thread(std::filesystem::path db_path, std::unique_ptr<SQLite::Database> db);
    void write_thread(std::filesystem::path db_path);
    void write_data(SQLite::Database& db, const ExcepData* data);
    void write_data(SQLite::Database& db, const DbAppOptionData* data);
    void write_data(SQLite::Database& db, const ItemData* data);
    void delete_data(SQLite::Database& db, const ItemData* data);
    void write_wf_meta(SQLite::Database& db, const WfData* data);
    void write_wf_blob(SQLite::Database& db, const WfBlob* data);
    void delete_wf_draft(SQLite::Database& db, const WfBlob* data);
    void purge_wf(SQLite::Database& db);

private:
    std::thread m_th;
    std::thread m_th_backup;
    std::mutex m_lock;
    std::condition_variable m_cv;
    std::atomic_bool m_stop_thread_flag = false;
    std::queue<WriteData> m_write_queue;

    std::list<WriteData> m_transaction_queue;
    std::atomic_bool m_transaction_flag = false;

    std::unique_ptr<SQLite::Database> m_db;
    bool m_first_run = true;
};

} // namespace mngr
