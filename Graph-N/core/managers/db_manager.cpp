#include "core/managers/db_manager.h"
#include "core/base/version.h"

namespace mngr
{
namespace
{
constexpr auto gdb_val_id = "id";
constexpr auto gdb_val_file = "file";
constexpr auto gdb_val_line = "line";
constexpr auto gdb_val_message = "message";
constexpr auto gdb_val_name = "name";
constexpr auto gdb_val_when = "at";
constexpr auto gdb_val_hash = "hash";
constexpr auto gdb_val_int = "value_int";
constexpr auto gdb_val_str = "value_str";
constexpr auto gdb_val_opt = "option";
constexpr auto gdb_val_uuid = "uuid";
constexpr auto gdb_val_type = "type";
constexpr auto gdb_val_pending_delete = "pending_delete";
constexpr auto gdb_val_saved = "saved";
constexpr auto gdb_val_saved_at = "saved_at";
constexpr auto gdb_val_draft = "draft";
constexpr auto gdb_val_draft_at = "draft_at";
constexpr auto gdb_val_flow = "flow";
constexpr auto gdb_val_prompt_id = "prompt_id";
constexpr auto gdb_val_data = "data";

constexpr auto gdb_valname_app_ver = "app_ver";
constexpr auto gdb_valname_last_run = "last_run";

constexpr auto gdb_tbl_meta = "METADATA";
constexpr auto gdb_tbl_err = "ERRORS";
constexpr auto gdb_tbl_opt = "SETTINGS";
constexpr auto gdb_tbl_items = "ITEMPROPS";
constexpr auto gdb_tbl_wfs = "WORKFLOWS";
constexpr auto gdb_tbl_history = "HISTORY";
constexpr auto gdb_tbl_history_search = "HISTORY_SEARCH";
} // namespace

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

_______________________________________________________________________________________________________________________________________________________________________
HISTORY:       | id       | prompt_id       | flow             | data
_______________|--------------------------------------------------------------------------------------------------------------------------------------------------------
               | int      | str             | blob             | str
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
               | prim     | task prompt id  | serialized flow  | prompt data in json
_______________________________________________________________________________________________________________________________________________________________________

_______________________________________________________________________________________________________________________________________________________________________
HISTORY_SEARCH:   | id                      | data
__________________|--------------------------------------------------------------------------------------------------------------------------------------------------------
                  | int                     | str
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
                  | link to HISTORY.id      | search data
_______________________________________________________________________________________________________________________________________________________________________

*/

#define LOG_VALUE(v)                   "(" << v.ival << ", " << v.sval << ")"
#define LOG_KEY(k)                     "opt: '" << k << "'"
#define LOG_KEY_VAL(k, v)              LOG_KEY(k) << " = " << LOG_VALUE(v)
#define LOG_NOPT_KEY(h, k)             "id=" << h << ", opt: '" << cast::to_i32(k)
#define LOG_NOPT_KEY_VAL(h, k, v)      LOG_NOPT_KEY(h, k) << "' = " << LOG_VALUE(v)
#define LOG_EXP(e)                     "err: (" << e.getErrorCode() << ", " << e.getErrorStr() << ", " << ex.what() << ")"

DbManager::DbManager() = default;
DbManager::~DbManager() = default;

void DbManager::stop_all_threads()
{
    m_cv.notify_all();
    if (m_th_write.joinable())
        m_th_write.join();
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
        SQLite::Database db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        meta.db_ver = db.execAndGet("PRAGMA user_version");

        SQLite::Statement query(db,
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
        query_lastrun.bind(2, tools::dt::from_current_date().as_full_date(false));
        query_lastrun.bind(3, 0);
        query_lastrun.exec();

        m_db->exec(fmt::format(FMT_STRING(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT,
						{} INTEGER,
						{} TEXT,
                        {} INTEGER
					);
			   )"), gdb_tbl_err, gdb_val_id,
                    gdb_val_file, gdb_val_line, gdb_val_message,
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

        m_db->exec(fmt::format(FMT_STRING(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
                        {} TEXT,
						{} BLOB,
                        {} TEXT
					);
			   )"), gdb_tbl_history, gdb_val_id,
                    gdb_val_prompt_id, gdb_val_flow, gdb_val_data));

        m_db->exec(fmt::format(FMT_STRING(R"(
					CREATE VIRTUAL TABLE IF NOT EXISTS {} USING fts5 (
						{}
					);
			   )"), gdb_tbl_history_search, 
                    gdb_val_data));

        transaction.commit();

        m_th_write = std::thread(&DbManager::write_thread, this, path);
        m_th_backup = std::thread(&DbManager::backup_thread, this, path, std::move(backup_db));
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("SQLite initialization " << LOG_EXP(ex));
        return false;
    }

    Log("Database initialized: " << path);
    return true;
}

void DbManager::begin_transaction()
{
    LogAssert(!m_transaction_flag.load(std::memory_order_relaxed), "Transaction already started");    
    m_transaction_flag.store(true, std::memory_order_relaxed);
}

void DbManager::end_transaction()
{
    LogAssert(m_transaction_flag.load(std::memory_order_relaxed), "Transaction wasn't started");
    m_transaction_flag.store(false, std::memory_order_relaxed);
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

void DbManager::exception_save(std::string&& message, const char* file, int32_t line) noexcept
{
    ExcepData data;
    //data.category = category;
    data.file = file;
    data.line = line;
    data.message = std::move(message);

    try
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_exception_write_data = std::move(data);
        m_exception_write_data_flag.store(true, std::memory_order_relaxed);
    }
    catch(...) {}
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
        if (!m_transaction_flag.load(std::memory_order_relaxed))
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag.load(std::memory_order_relaxed))
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
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull() && query.getColumn(2).isNull()) [[unlikely]]
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
        LogSqlDbErr("Load all options " << LOG_EXP(ex));
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
            if (!m_transaction_flag.load(std::memory_order_relaxed))
                m_write_queue.push(std::move(data));
            else
                m_transaction_queue.push_back(std::move(data));
        }
    }
    if (!m_transaction_flag.load(std::memory_order_relaxed))
        m_cv.notify_all();
}

void DbManager::item_option_save(hid hash, DbItemOptionType type, DbValue&& value)
{
    WriteData data;
    ItemData item;
    item.hash = hash;
    item.type = type;
    item.value = std::move(value);

    data.type = WriteType::Item;
    data.data = std::move(item);

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag.load(std::memory_order_relaxed))
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag.load(std::memory_order_relaxed))
        m_cv.notify_all();
}

std::optional<DbValue> DbManager::item_option_load(hid hash, DbItemOptionType type)
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

        query.bind(1, std::to_string(hash));
        query.bind(2, cast::to_i32(type));

        if (!query.executeStep()) [[unlikely]]
            return std::nullopt;
        if (query.getColumn(0).isNull() && query.getColumn(1).isNull()) [[unlikely]]
            return std::nullopt;

        std::optional<DbValue> v;
        v.emplace(query.getColumn(0).getString(), query.getColumn(1).getInt64());
        return v;
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Load opt " << LOG_NOPT_KEY(hash, type) << ", " << LOG_EXP(ex));
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
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull() && query.getColumn(2).isNull()) [[unlikely]]
                continue;

            const std::string hash = query.getColumn(0).getString();
            DbValue v(query.getColumn(1).getString(), query.getColumn(2).getInt());
            ret.emplace_back(std::strtoull(hash.c_str(), nullptr, 10), std::move(v));
        }

        return ret;
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Load all opts " << LOG_EXP(ex));
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
            if (query.getColumn(0).isNull()) [[unlikely]]
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
        LogSqlDbErr("Load all workflows " << LOG_EXP(ex));
    }

    WriteData data;
    data.type = WriteType::WfPurge;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag.load(std::memory_order_relaxed))
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag.load(std::memory_order_relaxed))
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
        if (!query.executeStep()) [[unlikely]]
            return;
        if (query.getColumn(0).isNull() && query.getColumn(1).isNull())
        {
            load_saved = true;
            return;
        }

        const auto saved_len = query.getColumn(0).getBytes();
        const auto draft_len = query.getColumn(1).getBytes();

        if (!load_saved && 0 == draft_len)
            load_saved = true;
        if (load_saved && 0 == saved_len) [[unlikely]]
            return;

        const void* ptr = query.getColumn(load_saved ? 0 : 1).getBlob();
        blob = blob::Blob(ptr, load_saved ? saved_len : draft_len);
        Log("Workflow blob loaded: " << uuid << ", b=" << (load_saved ? saved_len : draft_len) << " bytes, loaded " << (load_saved ? "stable" : "draft") << " version");
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Load workflow blob " << uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::wf_update_meta(const WfData& wf)
{
    WriteData data;
    data.type = WriteType::WfMeta;
    data.data = wf;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag.load(std::memory_order_relaxed))
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag.load(std::memory_order_relaxed))
        m_cv.notify_all();
}

void DbManager::wf_update_blob(const std::string& uuid, blob::Blob&& blob, bool draft, tools::dt time /*= {}*/)
{
    WfBlob wf;
    wf.uuid = uuid;
    wf.draft = draft;
    wf.blob = std::move(blob);
    wf.time = time;

    WriteData data;
    data.data = std::move(wf);
    data.type = WriteType::WfBlob;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag.load(std::memory_order_relaxed))
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag.load(std::memory_order_relaxed))
        m_cv.notify_all();
}

void DbManager::history_add(ExecTask& task)
{
    HistoryData h;
    h.blob = task.flow.copy();
    h.prompt_id = task.prompt_id;
    h.search_data = std::move(task.search_data);

    u_json j;
    j["prompt_id"] = task.prompt_id;
    j["run_no"] = task.run_no;
    j["err_message"] = task.err_message;
    j["err_full"] = task.err_full;
    j["time_start"] = task.time_start;
    j["time_start_s"] = task.time_start_s;
    j["time_end"] = task.time_end;
    j["time_ms"] = task.time_ms;

    switch (task.result)
    {
    case TaskResult::Failed:
        j["result"] = "failed";
        break;
    case TaskResult::Interrupted:
        j["result"] = "interrupted";
        break;
    case TaskResult::Success:
        j["result"] = "success";
        break;
    default:
        j["result"] = "";
    }

    auto jouts = u_json::array();
    for (auto& o : task.outputs)
    {
        u_json jout;
        switch (o.type)
        {
        case ExecTask::OutputType::Audio:
            jout["type"] = "audio";
            break;
        case ExecTask::OutputType::Text:
            jout["type"] = "text";
            break;
        case ExecTask::OutputType::Gif:
            jout["type"] = "gif";
            break;
        case ExecTask::OutputType::Image:
            jout["type"] = "image";
            break;
        default:
            jout["type"] = "";
        }

        jout["filename"] = o.filename;
        jout["filetype"] = o.filetype;
        jout["subfolder"] = o.subfolder;
        jout["format"] = o.format;
        jout["texts"] = o.text;

        jouts.push_back(std::move(jout));
    }

    j["assets"] = std::move(jouts);
    h.flow_data = j.dump();

    WriteData data;
    data.data = std::move(h);
    data.type = WriteType::HistoryAdd;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag.load(std::memory_order_relaxed))
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag.load(std::memory_order_relaxed))
        m_cv.notify_all();
}

std::vector<ExecTask> DbManager::history_load_all()
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE(R"(
                        SELECT {}, {}, {}
                        FROM {}
                )"),
                gdb_val_prompt_id, gdb_val_flow, gdb_val_data,
                gdb_tbl_history));

        std::vector<ExecTask> ret;
        ret.reserve(500);
        while (query.executeStep())
        {
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull() || query.getColumn(2).isNull()) [[unlikely]]
                continue;

            ExecTask task;
            task.loaded_from_db = true;
            task.group_id = base::GlobalID::random();
            task.task_id = base::GlobalID::random();

            try
            {
                const auto data = query.getColumn(2).getString();
                const auto j = u_json::parse(data);
                task.prompt_id = j["prompt_id"];
                task.run_no = j["run_no"];
                task.err_message = j["err_message"];
                task.err_full = j["err_full"];
                task.time_start = j["time_start"];
                task.time_start_s = j["time_start_s"];
                task.time_end = j["time_end"];
                task.time_ms = j["time_ms"];

                const std::string result = j["result"];                
                if (result == "interrupted") task.result = TaskResult::Interrupted;
                else if (result == "success") task.result = TaskResult::Success;
                else task.result = TaskResult::Failed;

                auto& jassets = j["assets"];
                for (auto& jitem : jassets)
                {
                    ExecTask::Output o;
                    const std::string type = jitem["type"];
                    if (type == "audio") o.type = ExecTask::OutputType::Audio;
                    else if (type == "text") o.type = ExecTask::OutputType::Text;
                    else if (type == "gif") o.type = ExecTask::OutputType::Gif;
                    else if (type == "image") o.type = ExecTask::OutputType::Image;
                    else o.type = ExecTask::OutputType::Unknown;

                    o.filename = jitem["filename"];
                    o.filetype = jitem["filetype"];
                    o.subfolder = jitem["subfolder"];
                    o.format = jitem["format"];
                    o.id = base::GlobalID::random();

                    auto& jtexts = jitem["texts"];
                    o.text = jtexts.get<std::string>();

                    if (!o.filename.empty())
                    {
                        const auto temp_filename = fs::get_temp_filepath(tools::path::path_to_string(std::filesystem::path(o.filename).extension()));
                        if (!temp_filename.empty())
                            o.binary_path = tools::path::path_to_string(temp_filename);
                    }

                    task.outputs.push_back(std::move(o));
                }
            }
            catch (const std::exception& ex)
            {
                const auto prompt_id = query.getColumn(0).getString();
                LogSqlDbErr("Failed to parse wf " << prompt_id << ": err " << ex.what());
                continue;
            }

            const auto blob_len = query.getColumn(1).getBytes();
            const void* ptr = query.getColumn(1).getBlob();
            task.flow = blob::Blob(ptr, blob_len);
            
            ret.push_back(std::move(task));
        }

        Log("Loaded " << ret.size() << " workflows");

        const auto sort_comp = [](const mngr::ExecTask& left, const mngr::ExecTask& right)
        {
            if (0 == left.time_start) return false;
            if (0 == right.time_start) return true;
            return left.time_start > right.time_start;
        };
        std::ranges::sort(ret, sort_comp);
        return ret;
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Load all history " << LOG_EXP(ex));
    }
    return {};
}

void DbManager::history_delete(const std::string& prompt_id)
{
    HistoryData h;
    h.prompt_id = prompt_id;

    WriteData data;
    data.data = std::move(h);
    data.type = WriteType::HistoryDel;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        if (!m_transaction_flag.load(std::memory_order_relaxed))
            m_write_queue.push(std::move(data));
        else
            m_transaction_queue.push_back(std::move(data));
    }
    if (!m_transaction_flag.load(std::memory_order_relaxed))
        m_cv.notify_all();
}

void DbManager::history_search(const std::string& prompt, std::list<std::string>& wfs)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE(R"(
                SELECT {0}.{1}, {0}.{2}
                FROM {0}
                JOIN {3} ON {0}.{1} = {3}.rowid
                WHERE {3} MATCH ?;
            )"),
                gdb_tbl_history, gdb_val_id, gdb_val_prompt_id, gdb_tbl_history_search));

        query.bindNoCopy(1, prompt);

        while (query.executeStep())
        {
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull()) [[unlikely]]
                continue;

            auto prompt_id = query.getColumn(1).getString();
            wfs.push_back(std::move(prompt_id));
        }

        Log("Search prompt '" << prompt << "', found wfs: " << wfs.size());
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("History search " << LOG_EXP(ex));
    }
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
            std::ranges::sort(list, sort_callback);

            for (const auto& file : list)
            {
                tools::path::remove(file);
                if (--to_delete <= 0)
                    break;
            }
        }

        backup /= tools::dt::get_app_start_time().as_filename() + ".db";
        auto path = tools::str::u8_to_string(backup.generic_u8string());
        tools::str::replace_this(path, "'", "''");
        const std::string command = fmt::format(FMT_STRING("VACUUM INTO '{}';"), path);
        db->exec(command);
        Log("Db backed up to " << path);
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Backup " << LOG_EXP(ex));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
/////////// write thread                   //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void DbManager::write_data(const SQLite::Database& db, const ExcepData& data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_STRING(R"(
                    INSERT INTO {}
                                ({}, {}, {}, {})
                        VALUES  (?,  ?,  ?,  ?)
                )"),
                gdb_tbl_err,
                gdb_val_file, gdb_val_line, gdb_val_message, gdb_val_when));

        query.bindNoCopy(1, data.file);
        query.bind(2, data.line);
        query.bindNoCopy(3, data.message);
        query.bind(4, tools::dt::from_current_date().time());
        query.exec();
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Save exception data " << LOG_EXP(ex));
    }
}

void DbManager::write_data(const SQLite::Database& db, const DbAppOptionData* data)
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
        query.bind(4, tools::dt::from_current_date().time());
        query.exec();
        Log("Upd " << LOG_KEY_VAL(data->name, data->val));
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Save opt data " << LOG_KEY_VAL(data->name, data->val) << ", " << LOG_EXP(ex));
    }
}

void DbManager::write_data(const SQLite::Database& db, const ItemData* data)
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
        query.bind(5, tools::dt::from_current_date().time());
        query.exec();
        Log("Upd " << LOG_NOPT_KEY_VAL(data->hash, data->type, data->value));
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Save item data " << LOG_NOPT_KEY_VAL(data->hash, data->type, data->value) << ", " << LOG_EXP(ex));
    }
}

void DbManager::delete_data(const SQLite::Database& db, const ItemData* data)
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
        LogSqlDbErr("Delete item data " << LOG_NOPT_KEY_VAL(data->hash, data->type, data->value) << ", " << LOG_EXP(ex));
    }
}

void DbManager::write_wf_meta(const SQLite::Database& db, const WfData* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_STRING(R"(
                        INSERT INTO {0}
                               ({1}, {2}, {3})
                        VALUES (?,  ?,  ?)
                        ON CONFLICT({1}) DO UPDATE SET
                        {2} = excluded.{2}, {3} = excluded.{3}
                )"),
                gdb_tbl_wfs, 
                gdb_val_uuid, gdb_val_name, gdb_val_pending_delete
            ));

        query.bindNoCopy(1, data->uuid);
        query.bindNoCopy(2, data->name);
        query.bind(3, data->pending_delete);
        query.exec();
        Log("Workflow meta upd: '" << data->name << "', uuid=" << data->uuid << ", dl=" << data->pending_delete);
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Upd workflow " << data->name << "###" << data->uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::write_wf_blob(const SQLite::Database& db, const WfBlob* data)
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
        if (data->time.null())
            query.bind(2, tools::dt::from_current_date().time());
        else
            query.bind(2, data->time.time());
        query.bindNoCopy(3, data->uuid);
        query.exec();
        Log("Workflow blob upd: " << data->uuid << ", bl=" << data->blob->size() << " bytes");
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Upd workflow blob " << data->uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::delete_wf_draft(const SQLite::Database& db, const WfBlob* data)
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
                gdb_val_draft, gdb_val_draft_at,
                gdb_val_uuid
            ));

        query.bind(1);
        query.bind(2);
        query.bindNoCopy(3, data->uuid);
        query.exec();
        Log("Workflow draft blob clear: " << data->uuid << ", bl=" << data->blob->size() << " bytes");
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Delete draft wf blob " << data->uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::purge_wf(const SQLite::Database& db)
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

        Log("Purged wfs: " << num);
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Purge workflows " << LOG_EXP(ex));
    }
}

void DbManager::add_history(const SQLite::Database& db, const HistoryData* data)
{
    try
    {
        SQLite::Statement query1(db,
            fmt::format(FMT_STRING(R"(
                    INSERT INTO {}
                                ({}, {}, {})
                        VALUES  (?,  ?,  ?)
                )"),
                gdb_tbl_history,
                gdb_val_prompt_id, gdb_val_flow, gdb_val_data));

        query1.bindNoCopy(1, data->prompt_id);
        query1.bindNoCopy(2, cast::to_void(data->blob->data()), cast::to_i32(data->blob->size()));
        query1.bindNoCopy(3, data->flow_data);
        query1.exec();

        const int64_t last_id = db.getLastInsertRowid();

        SQLite::Statement query2(db,
            fmt::format(FMT_STRING(R"(
                    INSERT INTO {}
                                (rowid, {})
                        VALUES  (?,  ?)
                )"),
                gdb_tbl_history_search,
                gdb_val_data));

        query2.bind(1, last_id);
        query2.bindNoCopy(2, data->search_data);
        query2.exec();
        Log("History wf " << data->prompt_id << " saved");
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Save history " << LOG_EXP(ex));
    }
}

void DbManager::del_history(const SQLite::Database& db, const HistoryData* data)
{
    try
    {
        SQLite::Statement query1(db,
            fmt::format(FMT_COMPILE(R"(
                SELECT {}
                FROM {}
                WHERE {} = ?
            )"),
                gdb_val_id, gdb_tbl_history, gdb_val_prompt_id));

        query1.bindNoCopy(1, data->prompt_id);

        if (query1.executeStep() && !query1.getColumn(0).isNull())
        {
            const auto task_id = query1.getColumn(0).getInt64();

            SQLite::Statement query2(db,
                fmt::format(FMT_COMPILE(R"(
                DELETE FROM {}
                WHERE rowid = ?
            )"),
                    gdb_tbl_history_search));

            query2.bind(1, task_id);
            query2.exec();

            SQLite::Statement query3(db,
                fmt::format(FMT_COMPILE(R"(
                DELETE FROM {}
                WHERE {} = ?
            )"), 
                    gdb_tbl_history, gdb_val_id));

            query3.bind(1, task_id);
            query3.exec();

            Log("Wf history " << data->prompt_id << " deleted");
        }
    }
    catch (const SQLite::Exception& ex)
    {
        LogSqlDbErr("Delete history " << LOG_EXP(ex));
    }
}

void DbManager::write_thread(std::filesystem::path db_path)
{
    constexpr const auto wait_period = 5s;

    SQLite::Database db(db_path, SQLite::OPEN_READWRITE);
    db.exec("PRAGMA journal_mode = WAL;");
    db.exec("PRAGMA synchronous = NORMAL;");

    ExcepData except_data;
    bool except_data_flag = false;

    while(true)
    {
        std::queue<WriteData> q;
        {
            std::unique_lock<std::mutex> lock(m_lock);
            m_cv.wait_for(lock, wait_period, [this]() noexcept { return g::app_global_stop_all_threads.stop_requested() || !m_write_queue.empty(); });

            if (g::app_global_stop_all_threads.stop_requested() && m_write_queue.empty() && !m_exception_write_data_flag.load(std::memory_order_relaxed))
                break;

            if (m_write_queue.empty() && !m_exception_write_data_flag.load(std::memory_order_relaxed))
                continue;

            if (m_exception_write_data_flag.load(std::memory_order_relaxed))
            {
                except_data = std::move(m_exception_write_data);
                m_exception_write_data_flag.store(false, std::memory_order_relaxed);
                except_data_flag = true;
            }
            else
            {
                std::swap(q, m_write_queue);
            }
        }

        try
        {
            if (except_data_flag)
            {
                except_data_flag = false;
                write_data(db, except_data);
            }
            else
            {
                SQLite::Transaction transaction(db);

                while (!q.empty())
                {
                    auto& data = q.front();
                    switch (data.type)
                    {
                    case WriteType::AppOption:
                        write_data(db, std::get_if<DbAppOptionData>(&data.data));
                        break;
                    case WriteType::Item:
                    {
                        const auto* node_data = std::get_if<ItemData>(&data.data);
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
                        const auto* wf_data = std::get_if<WfBlob>(&data.data);
                        write_wf_blob(db, wf_data);
                        if (!wf_data->draft)
                            delete_wf_draft(db, wf_data);
                    }
                    break;
                    case WriteType::WfPurge:
                        purge_wf(db);
                        break;
                    case WriteType::HistoryAdd:
                        add_history(db, std::get_if<HistoryData>(&data.data));
                        break;
                    case WriteType::HistoryDel:
                        del_history(db, std::get_if<HistoryData>(&data.data));
                        break;
                    default:
                        LogError("Unhandled data type to write: " << cast::to_i32(data.type));
                    }
                    q.pop();
                }

                transaction.commit();
            }
        }
        catch (const SQLite::Exception& ex)
        {
            LogSqlDbErr("Commit " << LOG_EXP(ex));
        }
    }
}

} // namespace mngr
