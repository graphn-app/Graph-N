
#include "core/managers/db_manager.h"

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
inline constexpr auto gdb_val_json = "json";
inline constexpr auto gdb_val_uuid = "uuid";
inline constexpr auto gdb_val_nodes = "nodes";
inline constexpr auto gdb_val_created_at = "created_at";
inline constexpr auto gdb_val_updated_at = "updated_at";
inline constexpr auto gdb_val_labels = "labels";
inline constexpr auto gdb_val_history_steps = "history_steps";
inline constexpr auto gdb_val_history_pos = "history_pos";
inline constexpr auto gdb_val_deleted = "deleted";
inline constexpr auto gdb_val_keep = "keep";

inline constexpr auto gdb_tbl_err = "ERRORS";
inline constexpr auto gdb_tbl_opt = "SETTINGS";
inline constexpr auto gdb_tbl_nodes = "NODES";
inline constexpr auto gdb_tbl_wf = "WORKFLOWS";

/*
_______________________________________________________________________________________________________________________________________________________________
ERRORS: | id        | file           | line      | category                      | message              | at
________|------------------------------------------------------------------------------------------------------------------------------------------------------
        | int       | str            | int       | int                           | str                  | datetime
---------------------------------------------------------------------------------------------------------------------------------------------------------------
        | prim      | message source             | enum ExcepCategory            | err message          | error time
_______________________________________________________________________________________________________________________________________________________________


_______________________________________________________________________________________________________________________________________________________________
SETTINGS: | id        | name           | value_int      | value_str         | at
__________|----------------------------------------------------------------------------------------------------------------------------------------------------
          | int       | str            | int            | str               | datetime
---------------------------------------------------------------------------------------------------------------------------------------------------------------
          | prim      | option name    | option value, int or str           | create/update time
_______________________________________________________________________________________________________________________________________________________________


_______________________________________________________________________________________________________________________________________________________________
NODES:    | id        | hash                    | option            | value_int      | value_str    | at
__________|----------------------------------------------------------------------------------------------------------------------------------------------------
          | int       | str                     | int               | int            | str          | datetime
---------------------------------------------------------------------------------------------------------------------------------------------------------------
          | prim      | hid as string           | enum NodeOption   | option value, int or str      | create/update time
_______________________________________________________________________________________________________________________________________________________________


_______________________________________________________________________________________________________________________________________________________________________
WORKFLOWS:    | id        | uuid          | name            | json        | labels     | deleted | keep             | history_steps  | history_pos        | created_at    | updated_at
______________|--------------------------------------------------------------------------------------------------------------------------------------------------------
              | int       | str           | str             | str         | str        | int     | int              | int            | int                | datetime      | datetime
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
              | prim      | WF id as uuid | name (caption)  | json-view   | wf labels  | flag    | force not delete | history length | curr history pos   | created       | last save
                                                                                                 | usually if       |
                                                                                                 | history is empty |
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
}

bool DbManager::init()
{
    auto path = fs::get_root_directory_path();
    path /= APP_CAPTION_LOW ".db";
/*
    const auto make_trigger = [](const char* table, const char* when)
    {
        return fmt::format(R"(
					        CREATE TRIGGER trg_upd_{}
                            AFTER UPDATE ON {}
                                BEGIN
                                UPDATE {} SET {} = CURRENT_TIMESTAMP WHERE {} = NEW.{};
                            END;					
				)", table, table, table, when, gdb_val_id, gdb_val_id);
    };
*/
    try
    {
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

        m_db->exec(fmt::format(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT,
						{} INTEGER,
						{} TEXT,
						{} TEXT,
                        {} DATETIME DEFAULT CURRENT_TIMESTAMP
					);
				)", gdb_tbl_err, gdb_val_id,
                    gdb_val_file, gdb_val_line, gdb_val_category, gdb_val_message,
                    gdb_val_when));

        m_db->exec(fmt::format(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT NOT NULL UNIQUE,
						{} INTEGER,
						{} TEXT,
                        {} DATETIME DEFAULT CURRENT_TIMESTAMP
					);
				)", gdb_tbl_opt, gdb_val_id,
                    gdb_val_name, gdb_val_int, gdb_val_str,
                    gdb_val_when));

        //m_db->exec(make_trigger(gdb_tbl_opt, gdb_val_when));

        m_db->exec(fmt::format(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT,
						{} INTEGER,
						{} INTEGER,
						{} TEXT,
                        {} DATETIME DEFAULT CURRENT_TIMESTAMP,
                        UNIQUE({}, {}) ON CONFLICT REPLACE
					);
				)", gdb_tbl_nodes, gdb_val_id,
                    gdb_val_hash, gdb_val_opt, gdb_val_int, gdb_val_str,
                    gdb_val_when,
                    gdb_val_hash, gdb_val_opt));

        //m_db->exec(make_trigger(gdb_tbl_nodes, gdb_val_when));

        m_db->exec(fmt::format(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT NOT NULL UNIQUE,
						{} TEXT,
                        {} TEXT,
                        {} TEXT,
						{} INTEGER,
						{} INTEGER,
						{} INTEGER,
						{} INTEGER,
                        {} DATETIME DEFAULT CURRENT_TIMESTAMP
                        {} DATETIME DEFAULT CURRENT_TIMESTAMP
					);
				)", gdb_tbl_wf, gdb_val_id,
                    gdb_val_uuid, gdb_val_name, gdb_val_json, gdb_val_labels, gdb_val_history_steps, gdb_val_history_pos, gdb_val_deleted, gdb_val_keep,
                    gdb_val_created_at, gdb_val_updated_at));

        //m_db->exec(make_trigger(gdb_tbl_wf, gdb_val_updated_at));

        transaction.commit();

        m_stop_thread_flag = false;
        m_th = std::thread(&DbManager::write_thread, this, path);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite initialization " << LOG_EXP(ex));
        return false;
    }

    Log("Database initialized: " << path);
    return true;
}

void DbManager::exception_save(ExcepCategory category, std::string&& message, const char* file, int line)
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

void DbManager::option_save(const std::string& key, DbValue&& value)
{
    WriteData data;
    DbOption o;
    o.name = key;
    o.val = std::move(value);

    data.type = WriteType::Option;
    data.data = std::move(o);

    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_write_queue.push(std::move(data));
    }
    m_cv.notify_all();
}

std::vector<DbOption> DbManager::option_load_all()
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

        std::vector<DbOption> ret;
        ret.reserve(200);
        while (query.executeStep())
        {
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull() && query.getColumn(2).isNull())
                continue;

            DbOption db_opt;
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

void DbManager::option_save_all(std::vector<DbOption>&& opts)
{
    {
        std::lock_guard<std::mutex> lock(m_lock);
        for (auto& o : opts)
        {
            WriteData data;
            data.type = WriteType::Option;
            data.data = std::move(o);
            m_write_queue.push(std::move(data));
        }
    }
    m_cv.notify_all();
}

void DbManager::node_option_save(hid node_hash, NodeOption opt, DbValue&& value)
{
    WriteData data;
    NodeData n;
    n.node_hash = node_hash;
    n.opt = opt;
    n.value = std::move(value);

    data.type = WriteType::Node;
    data.data = std::move(n);

    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_write_queue.push(std::move(data));
    }
    m_cv.notify_all();
}

std::optional<DbValue> DbManager::node_option_load(hid node_hash, NodeOption opt)
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
                gdb_tbl_nodes,
                gdb_val_hash, gdb_val_opt));

        query.bind(1, std::to_string(node_hash));
        query.bind(2, cast::to_i32(opt));

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
        LogExcep(ExcepCategory::SQLite, "SQLite load " << LOG_NOPT_KEY(node_hash, opt) << ", " << LOG_EXP(ex));
    }
    return std::nullopt;
}

std::vector<std::tuple<hid, NodeOption, DbValue>> DbManager::node_option_load_all()
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE(R"(
                    SELECT {}, {}, {}, {}
                    FROM {}
                )"),
                gdb_val_hash, gdb_val_opt, gdb_val_str, gdb_val_int,
                gdb_tbl_nodes));

        std::vector<std::tuple<hid, NodeOption, DbValue>> ret;
        ret.reserve(100);
        while(query.executeStep())
        {
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull() || query.getColumn(2).isNull() && query.getColumn(3).isNull())
                continue;

            const std::string hash = query.getColumn(0).getString();
            const NodeOption opt = cast::to_enum<NodeOption>(query.getColumn(1).getInt());
            DbValue v(query.getColumn(2).getString(), query.getColumn(3).getInt());

            ret.emplace_back(std::strtoull(hash.c_str(), nullptr, 10), opt, std::move(v));
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
                    SELECT {}, {}, {}, {}, {}, {}, {}, {}, {}
                    FROM {}
                    WHERE {} = 0 AND ({} > 0 OR {} = 0 AND {} = 1)
                )"),
                gdb_val_uuid, gdb_val_name, gdb_val_labels, gdb_val_history_steps, gdb_val_history_pos, gdb_val_deleted, gdb_val_keep,
                gdb_val_created_at, gdb_val_updated_at,
                gdb_tbl_wf,
                gdb_val_deleted, gdb_val_history_steps, gdb_val_history_steps, gdb_val_keep));

        while(query.executeStep())
        {
            if (query.getColumn(0).isNull())
                continue;
            
            WfData wfd;
            wfd.uuid = query.getColumn(0).getString();
            wfd.name = query.getColumn(1).getString();
            wfd.labels_raw = query.getColumn(2).getString();
            wfd.history_steps = query.getColumn(3).getInt();
            wfd.history_pos = query.getColumn(4).getInt();
            wfd.deleted = query.getColumn(5).getInt() == 1;
            wfd.keep = query.getColumn(6).getInt() == 1;
            wfd.created_at = query.getColumn(7).getString();
            wfd.saved_at = query.getColumn(8).getString();

            Log("Workflow loaded: " << wfd.name << "###" << wfd.uuid << ", lb=" << wfd.labels_raw << ", hs=" << wfd.history_steps
                << ", hp=" << wfd.history_pos << ", d=" << wfd.deleted << ", ct=" << wfd.created_at << ", st=" << wfd.saved_at);
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
        m_write_queue.push(data);
    }
    m_cv.notify_all();

    return wfs;
}

void DbManager::wf_load_json(WfData& wf)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE(R"(
                    SELECT {}
                    FROM {}
                    WHERE {} = ?
                )"),
                gdb_val_json,
                gdb_tbl_wf,
                gdb_val_uuid));

        query.bindNoCopy(1, wf.uuid);
        if (!query.executeStep())
            return;
        if (query.getColumn(0).isNull())
            return;
        wf.json = query.getColumn(0).getString();
        Log("Workflow loaded: " << wf.name << "###" << wf.uuid << ", j=" << wf.json.size());
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite load json workflow " << wf.name << "###" << wf.uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::wf_update_meta(const WfData& wf)
{
    WriteData data;
    WfData w;
    w.name = wf.name;
    w.uuid = wf.uuid;
    w.history_pos = wf.history_pos;
    w.history_steps = wf.history_steps;
    w.deleted = wf.deleted;
    w.keep = wf.keep;
    w.labels_raw = wf.labels_raw;

    data.type = WriteType::WfMeta;
    data.data = std::move(w);

    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_write_queue.push(std::move(data));
    }
    m_cv.notify_all();
}

void DbManager::wf_update_json(WfData& wf)
{
    WriteData data;
    WfData w;
    std::swap(w.json, wf.json);

    data.type = WriteType::WfJson;
    data.data = std::move(w);

    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_write_queue.push(std::move(data));
    }
    m_cv.notify_all();
}

/////////////////////////////////////////////////////////////////////////////////////
/////////// wring thread                   //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void DbManager::write_data(SQLite::Database& db, const ExcepData* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_COMPILE(R"(
                    INSERT INTO {}
                                ({}, {}, {}, {})
                        VALUES  (?,  ?,  ?,  ?)
                )"),
                gdb_tbl_err,
                gdb_val_file, gdb_val_line, gdb_val_category, gdb_val_message));

        query.bindNoCopy(1, data->file);
        query.bind(2, data->line);
        query.bind(3, cast::to_i64(data->category));
        query.bindNoCopy(4, data->message);
        query.exec();
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save " << LOG_EXP(ex));
    }
}

void DbManager::write_data(SQLite::Database& db, const DbOption* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_COMPILE(R"(
                        INSERT INTO {}
                                ({}, {}, {})
                        VALUES  (?,  ?,  ?)
                        ON CONFLICT({}) DO UPDATE SET
                            {} = datetime('now')
                )"),
                gdb_tbl_opt,
                gdb_val_name, gdb_val_int, gdb_val_str,
                gdb_val_name,
                gdb_val_when));

        query.bindNoCopy(1, data->name);
        query.bind(2, data->val.ival);
        query.bindNoCopy(3, data->val.sval);
        query.exec();
        Log("Upd " << LOG_KEY_VAL(data->name, data->val));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save " << LOG_KEY_VAL(data->name, data->val) << ", " << LOG_EXP(ex));
    }
}

void DbManager::write_data(SQLite::Database& db, const NodeData* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_COMPILE(R"(
                        INSERT INTO {}
                                ({}, {}, {}, {})
                        VALUES  (?,  ?,  ?,  ?)
                        ON CONFLICT({}) DO UPDATE SET
                            {} = datetime('now')
                )"),
                gdb_tbl_nodes,
                gdb_val_hash, gdb_val_opt, gdb_val_int, gdb_val_str,
                gdb_val_hash,
                gdb_val_when));

        query.bind(1, std::to_string(data->node_hash));
        query.bind(2, cast::to_i32(data->opt));
        query.bind(3, data->value.ival);
        query.bindNoCopy(4, data->value.sval);
        query.exec();
        Log("Upd " << LOG_NOPT_KEY_VAL(data->node_hash, data->opt, data->value));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save " << LOG_NOPT_KEY_VAL(data->node_hash, data->opt, data->value) << ", " << LOG_EXP(ex));
    }
}

void DbManager::write_wf_meta(SQLite::Database& db, const WfData* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_COMPILE(R"(
                        INSERT INTO {}
                               ({}, {}, {}, {}, {}, {}, {})
                        VALUES (?,  ?,  ?,  ?,  ?,  ?,  ?)
                        ON CONFLICT({}) DO UPDATE SET
                            {} = excluded.{},
                            {} = datetime('now')
                )"),
                gdb_tbl_wf,
                gdb_val_uuid, gdb_val_name, gdb_val_labels, gdb_val_history_steps, gdb_val_history_pos, gdb_val_deleted, gdb_val_keep,
                gdb_val_uuid,
                gdb_val_json, gdb_val_json,
                gdb_val_updated_at
            ));

        query.bindNoCopy(1, data->uuid);
        query.bindNoCopy(2, data->name);
        query.bindNoCopy(3, data->labels_raw);
        query.bind(4, data->history_steps);
        query.bind(5, data->history_pos);
        query.bind(6, data->deleted);
        query.bind(7, data->keep);
        query.exec();
        Log("Workflow upd: " << data->name << "###" << data->uuid << ", lb=" << data->labels_raw << ", hs=" << data->history_steps << ", hp=" << data->history_pos << ", d=" << data->deleted);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite upd workflow " << data->name << "###" << data->uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::write_wf_json(SQLite::Database& db, const WfData* data)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_COMPILE(R"(
                    INSERT OR REPLACE INTO {}
                               ({}, {})
                        VALUES (?,  ?)
                        ON CONFLICT({}) DO UPDATE SET
                            {} = excluded.{}, {} = excluded.{}, {} = excluded.{}, {} = excluded.{}, {} = excluded.{}, {} = excluded.{},
                            {} = datetime('now')
                )"),
                gdb_tbl_wf,
                gdb_val_uuid, gdb_val_json,
                gdb_val_uuid,
                gdb_val_name, gdb_val_name, gdb_val_labels, gdb_val_labels,
                    gdb_val_history_steps, gdb_val_history_steps,
                    gdb_val_history_pos, gdb_val_history_pos,
                    gdb_val_deleted, gdb_val_deleted, gdb_val_keep, gdb_val_keep,
                gdb_val_updated_at
            ));

        query.bindNoCopy(1, data->uuid);
        query.bindNoCopy(2, data->json);
        query.exec();
        Log("Workflow upd: " << data->name << "###" << data->uuid << ", j=" << data->json.size());
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite upd json workflow " << data->name << "###" << data->uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::purge_wf(SQLite::Database& db)
{
    try
    {
        SQLite::Statement query(db,
            fmt::format(FMT_COMPILE(R"(
                    DELETE FROM {}
                        WHERE {} = 1 OR {} = 0 AND {} = 0
                )"),
                gdb_tbl_wf,
                gdb_val_deleted, gdb_val_history_steps, gdb_val_keep));

        const auto num = query.exec();

        Log("Purged " << num << " workflows");
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite purge workflows " << LOG_EXP(ex));
    }
}

void DbManager::write_thread(std::filesystem::path db_path)
{
    using namespace std::chrono_literals;

    SQLite::Database db(db_path, SQLite::OPEN_READWRITE);
    db.exec("PRAGMA journal_mode = WAL;");
    db.exec("PRAGMA synchronous = NORMAL;");

    while(true)
    {
        std::queue<WriteData> q;
        {
            std::unique_lock<std::mutex> lock(m_lock);
            m_cv.wait_for(lock, std::chrono::milliseconds(1s), [this] { return m_stop_thread_flag || !m_write_queue.empty(); });

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
                case WriteType::Option:
                    write_data(db, std::get_if<DbOption>(&data.data));
                    break;
                case WriteType::Node:
                    write_data(db, std::get_if<NodeData>(&data.data));
                    break;
                case WriteType::WfMeta:
                    write_wf_meta(db, std::get_if<WfData>(&data.data));
                    break;
                case WriteType::WfJson:
                    write_wf_json(db, std::get_if<WfData>(&data.data));
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
