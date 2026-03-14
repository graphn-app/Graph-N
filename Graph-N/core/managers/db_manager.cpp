
#include "core/managers/db_manager.h"
#include "core/base/version.h"

namespace mngr
{

inline constexpr auto gdb_val_id = "id";
inline constexpr auto gdb_val_file = "file";
inline constexpr auto gdb_val_line = "line";
inline constexpr auto gdb_val_category = "category";
inline constexpr auto gdb_val_message = "message";
inline constexpr auto gdb_val_name = "name";
inline constexpr auto gdb_val_when = "at";
inline constexpr auto gdb_val_hash = "hash";
inline constexpr auto gdb_val_int = "value_int";
inline constexpr auto gdb_val_str = "value_str";
inline constexpr auto gdb_val_opt = "option";
inline constexpr auto gdb_val_uuid = "uuid";
inline constexpr auto gdb_val_type = "type";
inline constexpr auto gdb_val_pending_delete = "pending_delete";
inline constexpr auto gdb_val_saved = "saved";
inline constexpr auto gdb_val_saved_at = "saved_at";
inline constexpr auto gdb_val_draft = "draft";
inline constexpr auto gdb_val_draft_at = "draft_at";

inline constexpr auto gdb_valname_app_ver = "app_ver";
inline constexpr auto gdb_valname_last_run = "last_run";

inline constexpr auto gdb_tbl_meta = "METADATA";
inline constexpr auto gdb_tbl_err = "ERRORS";
inline constexpr auto gdb_tbl_opt = "SETTINGS";
inline constexpr auto gdb_tbl_items = "ITEMPROPS";
inline constexpr auto gdb_tbl_wfs = "WORKFLOWS";

/*
_______________________________________________________________________________________________________________________________________________________________
METADATA: | id        | name           | val_int      | val_str
__________|------------------------------------------------------------------------------------------------------------------------------------------------------
          | int       | str            | int          | str
---------------------------------------------------------------------------------------------------------------------------------------------------------------
          | prim      | app_ver        | 0            | app version
          | prim      | last_run       | 0            | last run time 
_______________________________________________________________________________________________________________________________________________________________

_______________________________________________________________________________________________________________________________________________________________
ERRORS: | id        | file           | line      | category                      | message              | at
________|------------------------------------------------------------------------------------------------------------------------------------------------------
        | int       | str            | int       | int                           | str                  | time_t
---------------------------------------------------------------------------------------------------------------------------------------------------------------
        | prim      | message source             | enum ExcepCategory            | err message          | error time
_______________________________________________________________________________________________________________________________________________________________


_______________________________________________________________________________________________________________________________________________________________
SETTINGS: | id        | name           | value_int      | value_str         | at
__________|----------------------------------------------------------------------------------------------------------------------------------------------------
          | int       | str            | int            | str               | time_t
---------------------------------------------------------------------------------------------------------------------------------------------------------------
          | prim      | option name    | option value, int or str           | create/update time
_______________________________________________________________________________________________________________________________________________________________


_______________________________________________________________________________________________________________________________________________________________
ITEMPROPS:| id        | hash                    | type                  | value_int      | value_str    | at
__________|----------------------------------------------------------------------------------------------------------------------------------------------------
          | int       | str                     | int                   | int            | str          | time_t
---------------------------------------------------------------------------------------------------------------------------------------------------------------
          | prim      | hid as string           | enum DbItemOptionType | option value, int or str      | create/update time
_______________________________________________________________________________________________________________________________________________________________


_______________________________________________________________________________________________________________________________________________________________________
WORKFLOWS:     | id        | uuid          | name            | pending_delete | at                 | saved          | saved_at  | draft           | draft_at
_______________|--------------------------------------------------------------------------------------------------------------------------------------------------------
               | int       | str           | str             | int            | time_t             | blob           | time_t    | blob            | time_t
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
               | prim      | WF id as uuid | name (caption)  | flag to purge  | create/update time | approved state | saved time | autosaved copy | draft saved time
_______________________________________________________________________________________________________________________________________________________________________

*/

#define LOG_VALUE(v)                   "(" << v.ival << ", " << v.sval << ")"
#define LOG_KEY(k)                     "opt: '" << k << "'"
#define LOG_KEY_VAL(k, v)              LOG_KEY(k) << " = " << LOG_VALUE(v)
#define LOG_NOPT_KEY(h, k)             "id=" << h << ", opt: '" << cast::to_i32(k)
#define LOG_NOPT_KEY_VAL(h, k, v)      LOG_NOPT_KEY(h, k) << "' = " << LOG_VALUE(v)
#define LOG_EXP(e)                     "err: (" << e.getErrorCode() << ", " << e.getErrorStr() << ", " << ex.what()

DbManager::~DbManager()
{
    LogF;
}

void DbManager::stop_all_threads()
{
    m_stop_thread_flag = true;
    m_cv.notify_all();
    if (m_th.joinable())
        m_th.join();
    if (m_th_backup.joinable())
        m_th_backup.join();
}

DbMetadata DbManager::read_db_metadata()
{
    auto path = fs::get_root_directory_path();
    path /= APP_CAPTION_LOW ".db";
    if (!tools::path::exists(path))
        return {};

    DbMetadata meta;

    try
    {
        auto db = std::make_unique<SQLite::Database>(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        meta.db_ver = db->execAndGet("PRAGMA user_version");

        SQLite::Statement query(*db,
            fmt::format(FMT_COMPILE(R"(
                    SELECT {}, {}, {}
                    FROM {}
                )"),
                gdb_val_name, gdb_val_str, gdb_val_int,
                gdb_tbl_meta));

        while (query.executeStep())
        {
            if (query.getColumn(0).getString() == gdb_valname_app_ver)
                meta.app_ver = query.getColumn(1).getString();
            else if (query.getColumn(0).getString() == gdb_valname_last_run)
                meta.last_run = query.getColumn(1).getString();
        }

        return meta;
    }
    catch (const SQLite::Exception&)
    {
    }
    return {};
}

bool DbManager::init()
{
    LogF;
    if (!Manager::init())
        return false;

    auto path = fs::get_root_directory_path();
    path /= APP_CAPTION_LOW ".db";

    try
    {
        auto backup_db = std::make_unique<SQLite::Database>(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        m_db = std::make_unique<SQLite::Database>(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        m_db->exec("PRAGMA journal_mode = WAL;");
        m_db->exec("PRAGMA synchronous = NORMAL;");

        SQLite::Statement first_time_query(*m_db, 
            "SELECT count(*) FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
        if (first_time_query.executeStep())
        {
            m_first_run = (first_time_query.getColumn(0).getInt() == 0);
        }

        SQLite::Transaction transaction(*m_db);

        m_db->exec("PRAGMA user_version = " + std::to_string(ver::app_major) + ";");

        m_db->exec(fmt::format(FMT_STRING(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT NOT NULL UNIQUE,
						{} TEXT,
						{} INTEGER
					);
			   )"), gdb_tbl_meta, gdb_val_id,
                    gdb_val_name, gdb_val_str, gdb_val_int));

        SQLite::Statement query_appver(*m_db,
            fmt::format(FMT_STRING(R"(
                        REPLACE INTO {}
                                ({}, {}, {})
                        VALUES  (?,  ?,  ?)
                )"),
                gdb_tbl_meta,
                gdb_val_name, gdb_val_str, gdb_val_int));
        query_appver.bindNoCopy(1, gdb_valname_app_ver);
        query_appver.bind(2, ver::ver_as_string());
        query_appver.bind(3, 0);
        query_appver.exec();

        SQLite::Statement query_lastrun(*m_db,
            fmt::format(FMT_STRING(R"(
                        REPLACE INTO {}
                                ({}, {}, {})
                        VALUES  (?,  ?,  ?)
                )"),
                gdb_tbl_meta,
                gdb_val_name, gdb_val_str, gdb_val_int));
        query_lastrun.bindNoCopy(1, gdb_valname_last_run);
        query_lastrun.bind(2, tools::dt::from_current_date().as_full_date());
        query_lastrun.bind(3, 0);
        query_lastrun.exec();

        m_db->exec(fmt::format(FMT_STRING(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT,
						{} INTEGER,
						{} TEXT,
						{} TEXT,
                        {} INTEGER
					);
			   )"), gdb_tbl_err, gdb_val_id,
                    gdb_val_file, gdb_val_line, gdb_val_category, gdb_val_message,
                    gdb_val_when));

        m_db->exec(fmt::format(FMT_STRING(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT NOT NULL UNIQUE,
						{} INTEGER,
						{} TEXT,
                        {} INTEGER
					);
			   )"), gdb_tbl_opt, gdb_val_id,
                    gdb_val_name, gdb_val_int, gdb_val_str,
                    gdb_val_when));

        m_db->exec(fmt::format(FMT_STRING(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT,
						{} INTEGER,
						{} INTEGER,
						{} TEXT,
                        {} INTEGER,
                        UNIQUE({}, {}) ON CONFLICT REPLACE
					);
			   )"), gdb_tbl_items, gdb_val_id,
                    gdb_val_hash, gdb_val_type, gdb_val_int, gdb_val_str,
                    gdb_val_when,
                    gdb_val_hash, gdb_val_type));

        m_db->exec(fmt::format(FMT_STRING(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT NOT NULL UNIQUE,
						{} TEXT,
						{} INTEGER,
                        {} INTEGER,
						{} BLOB,
                        {} INTEGER,
						{} BLOB,
                        {} INTEGER
					);
			   )"), gdb_tbl_wfs, gdb_val_id,
                    gdb_val_uuid, gdb_val_name, gdb_val_pending_delete, gdb_val_when,
                    gdb_val_saved, gdb_val_saved_at, gdb_val_draft, gdb_val_draft_at));

        transaction.commit();

        m_stop_thread_flag = false;
        m_th = std::thread(&DbManager::write_thread, this, path);
        m_th_backup = std::thread(&DbManager::backup_thread, this, path, std::move(backup_db));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite initialization " << LOG_EXP(ex));
        return false;
    }

    Log("Database initialized: " << path);
    return true;
}

void DbManager::begin_transaction()
{
    if (m_transaction_flag)
        LogWarn("Transaction already started");
    m_transaction_flag = true;
}

void DbManager::end_transaction()
{
    if (!m_transaction_flag)
    {
        LogInternalLogic("Transaction wasn't started");
        return;
    }

    m_transaction_flag = false;
    if (m_transaction_queue.empty())
        return;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        for (auto& tra : m_transaction_queue)
        {
            m_write_queue.push(std::move(tra));
        }
        m_transaction_queue.clear();
    }
    m_cv.notify_all();
}

void DbManager::exception_save(ExcepCategory category, std::string&& message, const char* file, int32_t line)
{
    WriteData data;
    ExcepData e;
    e.category = category;
    e.file = file;
    e.line = line;
    e.message = std::move(message);

    data.type = WriteType::Exception;
    data.data = std::move(e);

    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_write_queue.push(std::move(data));
    }
    m_cv.notify_all();
}

void DbManager::app_option_save(const std::string& key, DbValue&& value)
{
    WriteData data;
    DbAppOptionData o;
    o.name = key;
    o.val = std::move(value);

    data.type = WriteType::AppOption;
    data.data = std::move(o);

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag)
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag)
        m_cv.notify_all();
}

std::vector<DbAppOptionData> DbManager::app_option_load_all()
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE(R"(
                        SELECT {}, {}, {}
                        FROM {}
                )"),
                gdb_val_name, gdb_val_str, gdb_val_int,
                gdb_tbl_opt));

        std::vector<DbAppOptionData> ret;
        ret.reserve(200);
        while (query.executeStep())
        {
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull() && query.getColumn(2).isNull())
                continue;

            DbAppOptionData db_opt;
            db_opt.name = query.getColumn(0).getString();
            db_opt.val.sval = query.getColumn(1).getString();
            db_opt.val.ival = query.getColumn(2).getInt64();
            ret.push_back(std::move(db_opt));
        }

        return ret;
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite load all options " << LOG_EXP(ex));
    }
    return {};
}

void DbManager::app_option_save_all(std::vector<DbAppOptionData>&& opts)
{
    {
        std::lock_guard<std::mutex> lock(m_lock);
        for (auto& o : opts)
        {
            WriteData data;
            data.type = WriteType::AppOption;
            data.data = std::move(o);
            if (!m_transaction_flag)
                m_write_queue.push(std::move(data));
            else
                m_transaction_queue.push_back(std::move(data));
        }
    }
    if (!m_transaction_flag)
        m_cv.notify_all();
}

void DbManager::item_option_save(hid id, DbItemOptionType type, DbValue&& value)
{
    WriteData data;
    ItemData item;
    item.hash = id;
    item.type = type;
    item.value = std::move(value);

    data.type = WriteType::Item;
    data.data = std::move(item);

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag)
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag)
        m_cv.notify_all();
}

std::optional<DbValue> DbManager::item_option_load(hid id, DbItemOptionType type)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE(R"(
                    SELECT {}, {}
                    FROM {}
                    WHERE {}=? AND {}=?
                )"),
                gdb_val_str, gdb_val_int,
                gdb_tbl_items,
                gdb_val_hash, gdb_val_type));

        query.bind(1, std::to_string(id));
        query.bind(2, cast::to_i32(type));

        if (!query.executeStep())
            return std::nullopt;
        if (query.getColumn(0).isNull() && query.getColumn(1).isNull())
            return std::nullopt;

        std::optional<DbValue> v;
        v.emplace(query.getColumn(0).getString(), query.getColumn(1).getInt64());
        return v;
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite load " << LOG_NOPT_KEY(id, type) << ", " << LOG_EXP(ex));
    }
    return std::nullopt;
}

std::vector<std::pair<hid, DbValue>> DbManager::item_option_load_all(DbItemOptionType type)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE(R"(
                    SELECT {}, {}, {}
                    FROM {}
                    WHERE {} = ?
                )"),
                gdb_val_hash, gdb_val_str, gdb_val_int,
                gdb_tbl_items,
                gdb_val_type));
        query.bind(1, cast::to_i32(type));

        std::vector<std::pair<hid, DbValue>> ret;
        ret.reserve(100);
        while(query.executeStep())
        {
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull() && query.getColumn(2).isNull())
                continue;

            const std::string hash = query.getColumn(0).getString();
            DbValue v(query.getColumn(1).getString(), query.getColumn(2).getInt());
            ret.emplace_back(std::strtoull(hash.c_str(), nullptr, 10), std::move(v));
        }

        return ret;
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite load all opts " << LOG_EXP(ex));
    }
    return {};
}

std::vector<WfData> DbManager::wf_load_meta_and_purge()
{
    std::vector<WfData> wfs;
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE(R"(
                        SELECT {}, {}, LENGTH({}), {}, LENGTH({}), {}
                        FROM {}
                        WHERE {} = 0
                    )"),
                gdb_val_uuid, gdb_val_name, gdb_val_saved, gdb_val_saved_at, gdb_val_draft, gdb_val_draft_at,
                gdb_tbl_wfs,
                gdb_val_pending_delete));

        while (query.executeStep())
        {
            if (query.getColumn(0).isNull())
                continue;

            WfData wfd;
            wfd.uuid = query.getColumn(0).getString();
            wfd.name = query.getColumn(1).getString();
            wfd.saved_exist = query.getColumn(2).getInt() > 0;
            wfd.saved_time = tools::dt::from_time(query.getColumn(3).getInt64());
            wfd.draft_exist = query.getColumn(4).getInt() > 0;
            wfd.draft_time = tools::dt::from_time(query.getColumn(5).getInt64());

            Log("Workflow loaded: " << wfd.name << "###" << wfd.uuid);
            wfs.push_back(wfd);
        }

        Log("Loaded " << wfs.size() << " workflows");
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite load all workflows, " << LOG_EXP(ex));
    }

    WriteData data;
    data.type = WriteType::WfPurge;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag)
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag)
        m_cv.notify_all();

    return wfs;
}

void DbManager::wf_load_blob(const std::string& uuid, blob::Blob& blob, bool& load_saved)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE(R"(
                    SELECT {}, {}
                    FROM {}
                    WHERE {} = ?
                )"),
                gdb_val_saved, gdb_val_draft,
                gdb_tbl_wfs,
                gdb_val_uuid));

        query.bindNoCopy(1, uuid);
        if (!query.executeStep())
            return;
        if (query.getColumn(0).isNull() && query.getColumn(1).isNull())
        {
            load_saved = true;
            return;
        }

        auto saved_len = query.getColumn(0).getBytes();
        auto draft_len = query.getColumn(1).getBytes();

        if (!load_saved && 0 == draft_len)
            load_saved = true;
        if (load_saved && 0 == saved_len)
            return;

        const void* ptr = query.getColumn(load_saved ? 0 : 1).getBlob();
        blob = blob::Blob(ptr, load_saved ? saved_len : draft_len);
        Log("Workflow loaded: " << uuid << ", b=" << (load_saved ? saved_len : draft_len) << " bytes, loaded " << (load_saved ? "saved" : "draft") << " copy");
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite load blob workflow " << uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::wf_update_meta(const WfData& wf)
{
    WriteData data;
    data.type = WriteType::WfMeta;
    data.data = wf;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag)
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag)
        m_cv.notify_all();
}

void DbManager::wf_update_blob(const std::string& uuid, blob::Blob&& blob, bool draft)
{
    WfBlob wf;
    wf.uuid = uuid;
    wf.draft = draft;
    wf.blob = std::move(blob);

    WriteData data;
    data.data = std::move(wf);
    data.type = WriteType::WfBlob;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag)
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag)
        m_cv.notify_all();
}

/////////////////////////////////////////////////////////////////////////////////////
/////////// backup thread                   //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void DbManager::backup_thread(std::filesystem::path db_path, std::unique_ptr<SQLite::Database> db)
{
    try
    {
        if (db_path.empty())
            return;

        auto backup = fs::get_backup_directory_path();
        if (!tools::path::make_folder(backup))
            return;

        auto list = tools::path::enumerate_directory(backup, false, false, true, false);
        constexpr int32_t to_save = 2;
        int32_t to_delete = cast::to_i32(list.size()) - to_save;
        if (to_delete > 0)
        {
            const auto sort_callback = [](const std::filesystem::path& lhs, const std::filesystem::path& rhs)
            {
                return lhs.wstring() < rhs.wstring();
            };
            std::sort(list.begin(), list.end(), sort_callback);

            for (const auto& file : list)
            {
                tools::path::remove(file);
                if (--to_delete <= 0)
                    break;
            }
        }

        backup /= tools::dt::get_app_start_time().as_filename() + ".db";
        auto path = backup.generic_u8string();
        tools::str::replace_this(path, "'", "''");
        const std::string command = fmt::format(FMT_STRING("VACUUM INTO '{}';"), path);
        db->exec(command);
        Log("Db backuped up to " << path);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite backup err: " << LOG_EXP(ex));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
/////////// write thread                   //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void DbManager::write_data(SQLite::Database& db, const ExcepData* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_STRING(R"(
                    INSERT INTO {}
                                ({}, {}, {}, {}, {})
                        VALUES  (?,  ?,  ?,  ?,  ?)
                )"),
                gdb_tbl_err,
                gdb_val_file, gdb_val_line, gdb_val_category, gdb_val_message, gdb_val_when));

        query.bindNoCopy(1, data->file);
        query.bind(2, data->line);
        query.bind(3, cast::to_i64(data->category));
        query.bindNoCopy(4, data->message);
        query.bind(5, tools::dt::from_current_date().raw());
        query.exec();
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save " << LOG_EXP(ex));
    }
}

void DbManager::write_data(SQLite::Database& db, const DbAppOptionData* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_STRING(R"(
                        REPLACE INTO {}
                                ({}, {}, {}, {})
                        VALUES  (?,  ?,  ?,  ?)
                )"),
                gdb_tbl_opt,
                gdb_val_name, gdb_val_int, gdb_val_str, gdb_val_when));

        query.bindNoCopy(1, data->name);
        query.bind(2, data->val.ival);
        query.bindNoCopy(3, data->val.sval);
        query.bind(4, tools::dt::from_current_date().raw());
        query.exec();
        Log("Upd " << LOG_KEY_VAL(data->name, data->val));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save " << LOG_KEY_VAL(data->name, data->val) << ", " << LOG_EXP(ex));
    }
}

void DbManager::write_data(SQLite::Database& db, const ItemData* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_STRING(R"(
                        REPLACE INTO {}
                                ({}, {}, {}, {}, {})
                        VALUES  (?,  ?,  ?,  ?,  ?)
                )"),
                gdb_tbl_items,
                gdb_val_hash, gdb_val_type, gdb_val_int, gdb_val_str, gdb_val_when));

        query.bind(1, std::to_string(data->hash));
        query.bind(2, cast::to_i32(data->type));
        query.bind(3, data->value.ival);
        query.bindNoCopy(4, data->value.sval);
        query.bind(5, tools::dt::from_current_date().raw());
        query.exec();
        Log("Upd " << LOG_NOPT_KEY_VAL(data->hash, data->type, data->value));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save " << LOG_NOPT_KEY_VAL(data->hash, data->type, data->value) << ", " << LOG_EXP(ex));
    }
}

void DbManager::delete_data(SQLite::Database& db, const ItemData* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_STRING(R"(
                        DELETE FROM {}
                            WHERE {} = ? AND {} = ?
                )"),
                gdb_tbl_items,
                gdb_val_hash, gdb_val_type));

        query.bind(1, std::to_string(data->hash));
        query.bind(2, cast::to_i32(data->type));
        query.exec();
        Log("Del " << LOG_NOPT_KEY(data->hash, data->type));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save " << LOG_NOPT_KEY_VAL(data->hash, data->type, data->value) << ", " << LOG_EXP(ex));
    }
}

void DbManager::write_wf_meta(SQLite::Database& db, const WfData* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_STRING(R"(
                        INSERT INTO {}
                               ({}, {}, {})
                        VALUES (?,  ?,  ?)
                        ON CONFLICT({}) DO UPDATE SET
                        {} = excluded.{}, {} = excluded.{}
                )"),
                gdb_tbl_wfs,
                gdb_val_uuid, gdb_val_name, gdb_val_pending_delete,
                gdb_val_uuid,
                gdb_val_name, gdb_val_name, gdb_val_pending_delete, gdb_val_pending_delete
            ));

        query.bindNoCopy(1, data->uuid);
        query.bindNoCopy(2, data->name);
        query.bind(3, data->pending_delete);
        query.exec();
        Log("Workflow meta upd: '" << data->name << "', uuid=" << data->uuid << ", dl=" << data->pending_delete);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite upd workflow " << data->name << "###" << data->uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::write_wf_blob(SQLite::Database& db, const WfBlob* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_STRING(R"(
                        UPDATE {}
                        SET {} = ?, {} = ?
                        WHERE {} = ?
                )"),
                gdb_tbl_wfs,
                (data->draft ? gdb_val_draft : gdb_val_saved),
                (data->draft ? gdb_val_draft_at : gdb_val_saved_at),
                gdb_val_uuid
            ));

        query.bindNoCopy(1, cast::to_void(data->blob->data()), cast::to_i32(data->blob->size()));
        query.bind(2, tools::dt::from_current_date().raw());
        query.bindNoCopy(3, data->uuid);
        query.exec();
        Log("Workflow blob upd: " << data->uuid << ", bl=" << data->blob->size() << " bytes");
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite upd blob workflow " << data->uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::delete_wf_draft(SQLite::Database& db, const WfBlob* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_STRING(R"(
                        UPDATE {}
                        SET {} = ?
                        WHERE {} = ?
                )"),
                gdb_tbl_wfs,
                gdb_val_draft,
                gdb_val_uuid
            ));

        query.bind(1);
        query.bindNoCopy(2, data->uuid);
        query.exec();
        Log("Workflow draft blob clear: " << data->uuid << ", bl=" << data->blob->size() << " bytes");
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite clear draft blob, wf=" << data->uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::purge_wf(SQLite::Database& db)
{
    try
    {
        SQLite::Statement query1(db,
            fmt::format(FMT_STRING(R"(
                    DELETE FROM {}
                    WHERE {} = 1
                )"),
                gdb_tbl_wfs,
                gdb_val_pending_delete
            ));

        const auto num = query1.exec();

        Log("Purged WFs: " << num);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite purge workflows " << LOG_EXP(ex));
    }
}

void DbManager::write_thread(std::filesystem::path db_path)
{
    using namespace std::chrono_literals;
    constexpr const auto wait_period = 5s;

    SQLite::Database db(db_path, SQLite::OPEN_READWRITE);
    db.exec("PRAGMA journal_mode = WAL;");
    db.exec("PRAGMA synchronous = NORMAL;");

    while(true)
    {
        std::queue<WriteData> q;
        {
            std::unique_lock<std::mutex> lock(m_lock);
            m_cv.wait_for(lock, std::chrono::milliseconds(wait_period), [this] { return m_stop_thread_flag || !m_write_queue.empty(); });

            if (m_stop_thread_flag && m_write_queue.empty())
                break;

            if (m_write_queue.empty())
                continue;

            std::swap(q, m_write_queue);
        }

        try
        {
            SQLite::Transaction transaction(db);

            while (!q.empty())
            {
                auto& data = q.front();
                switch (data.type)
                {
                case WriteType::Exception:
                    write_data(db, std::get_if<ExcepData>(&data.data));
                    break;
                case WriteType::AppOption:
                    write_data(db, std::get_if<DbAppOptionData>(&data.data));
                    break;
                case WriteType::Item:
                    {
                        auto node_data = std::get_if<ItemData>(&data.data);
                        if (node_data->value.empty())
                            delete_data(db, node_data);
                        else
                            write_data(db, node_data);                    
                    }
                    break;
                case WriteType::WfMeta:
                    write_wf_meta(db, std::get_if<WfData>(&data.data));
                    break;
                case WriteType::WfBlob:
                    {
                        auto wf_data = std::get_if<WfBlob>(&data.data);
                        write_wf_blob(db, wf_data);
                        if (!wf_data->draft)
                            delete_wf_draft(db, wf_data);
                    }
                    break;
                case WriteType::WfPurge:
                    purge_wf(db);
                    break;
                default:
                    LogError(ExcepCategory::Logic, "Unhandled data type to write: " << cast::to_i32(data.type));
                }
                q.pop();
            }

            transaction.commit();
        }
        catch (const SQLite::Exception& ex)
        {
            LogExcep(ExcepCategory::SQLite, "SQLite commit err: " << LOG_EXP(ex));
        }
    }
}

} // namespace mngr
