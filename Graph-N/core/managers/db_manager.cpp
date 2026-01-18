
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
inline constexpr auto gdb_val_open = "open";
inline constexpr auto gdb_val_fav = "favourite";
inline constexpr auto gdb_val_json = "json";
inline constexpr auto gdb_val_uuid = "uuid";
inline constexpr auto gdb_val_nodes = "nodes";

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


_______________________________________________________________________________________________________________________________________________________________
WORKFLOWS:    | id        | uuid                | name                     | open          | favourite     | json           | nodes         | at
______________|------------------------------------------------------------------------------------------------------------------------------------------------
              | int       | str                 | str                      | int           | int           | str            | int           | datetime
---------------------------------------------------------------------------------------------------------------------------------------------------------------
              | prim      | id of WF as uuid    | name (caption)           | is open       | is favourited | json-view      | num of nodes  | create/update time
_______________________________________________________________________________________________________________________________________________________________
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

bool DbManager::init()
{
    auto path = fs::get_root_directory_path();
    path /= APP_CAPTION_LOW ".db";
    const auto u8 = path.u8string();

    try
    {
        m_db = std::make_unique<SQLite::Database>(u8, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        m_db->exec("PRAGMA journal_mode = WAL;");
        m_db->exec("PRAGMA synchronous = NORMAL;");

        SQLite::Statement first_time_query(*m_db, 
            "SELECT count(*) FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
        if (first_time_query.executeStep())
        {
            m_first_run = (first_time_query.getColumn(0).getInt() == 0);
        }

        m_db->exec(fmt::format(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT,
						{} INTEGER,
						{} TEXT,
						{} TEXT,
                        {} DATETIME DEFAULT CURRENT_TIMESTAMP
					);
				)", gdb_tbl_err, gdb_val_id, gdb_val_file, gdb_val_line, gdb_val_category, gdb_val_message, gdb_val_when));

        m_db->exec(fmt::format(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT NOT NULL UNIQUE,
						{} INTEGER,
						{} TEXT,
                        {} DATETIME DEFAULT CURRENT_TIMESTAMP
					);
				)", gdb_tbl_opt, gdb_val_id, gdb_val_name, gdb_val_int, gdb_val_str, gdb_val_when));

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
				)", gdb_tbl_nodes, gdb_val_id, gdb_val_hash, gdb_val_opt, gdb_val_int, gdb_val_str, gdb_val_when,
                    gdb_val_hash, gdb_val_opt));

        m_db->exec(fmt::format(R"(
					CREATE TABLE IF NOT EXISTS {} (
						{} INTEGER PRIMARY KEY AUTOINCREMENT,
						{} TEXT NOT NULL UNIQUE,
						{} TEXT,
						{} INTEGER,
						{} INTEGER,
                        {} TEXT,
						{} INTEGER,
                        {} DATETIME DEFAULT CURRENT_TIMESTAMP
					);
				)", gdb_tbl_wf, gdb_val_id, gdb_val_uuid, gdb_val_name, gdb_val_open, gdb_val_fav, gdb_val_json, gdb_val_nodes, gdb_val_when));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite initialization " << LOG_EXP(ex));
        return false;
    }

    Log("Database initialized: " << path);
    return true;
}

void DbManager::exception_save(ExcepCategory category, const std::string& message, const char* file, int line)
{
    std::lock_guard<std::mutex> lock(m_lock);

    try
    {
        SQLite::Statement query(*m_db,
            fmt::format("INSERT INTO {} ({}, {}, {}, {}) VALUES (?, ?, ?, ?)",
                gdb_tbl_err, gdb_val_file, gdb_val_line, gdb_val_category, gdb_val_message));
        query.bindNoCopy(1, file);
        query.bind(2, line);
        query.bind(3, cast::to_i64(category));
        query.bindNoCopy(4, message);
        query.exec();
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save " << LOG_EXP(ex));
    }
}

void DbManager::option_save(const std::string& key, const DbValue& value)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("INSERT OR REPLACE INTO {} ({}, {}, {}) VALUES (?, ?, ?)"),
                gdb_tbl_opt, gdb_val_name, gdb_val_int, gdb_val_str));
        query.bind(1, key);
        query.bind(2, value.ival);
        query.bindNoCopy(3, value.sval);
        query.exec();
        Log("Upd " << LOG_KEY_VAL(key, value));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save " << LOG_KEY_VAL(key, value) << ", " << LOG_EXP(ex));
    }
}

std::vector<DbOption> DbManager::option_load_all()
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {}, {}, {} FROM {}"),
                gdb_val_name, gdb_val_str, gdb_val_int, gdb_tbl_opt));

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

void DbManager::option_save_all(const std::vector<DbOption>& opts)
{
    try
    {
        SQLite::Transaction transaction(*m_db);
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("INSERT OR REPLACE INTO {} ({}, {}, {}) VALUES (?, ?, ?)"),
                gdb_tbl_opt, gdb_val_name, gdb_val_int, gdb_val_str));

        for (auto& opt : opts)
        {
            query.bindNoCopy(1, opt.name);
            query.bind(2, opt.val.ival);
            query.bindNoCopy(3, opt.val.sval);

            query.exec();
            query.reset();
        }
        transaction.commit();
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save all options " << LOG_EXP(ex));
    }
}

void DbManager::node_option_save(hid node_hash, NodeOption opt, const DbValue& value)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("INSERT INTO {} ({}, {}, {}, {}) VALUES (?, ?, ?, ?)"),
                gdb_tbl_nodes, gdb_val_hash, gdb_val_opt, gdb_val_int, gdb_val_str));
        query.bind(1, std::to_string(node_hash));
        query.bind(2, cast::to_i32(opt));
        query.bind(3, value.ival);
        query.bindNoCopy(4, value.sval);
        query.exec();
        Log("Upd " << LOG_NOPT_KEY_VAL(node_hash, opt, value));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite save " << LOG_NOPT_KEY_VAL(node_hash, opt, value) << ", " << LOG_EXP(ex));
    }
}

std::optional<DbValue> DbManager::node_option_load(hid node_hash, NodeOption opt)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {}, {} FROM {} WHERE {}=? AND {}=?"),
                gdb_val_str, gdb_val_int, gdb_tbl_nodes, gdb_val_hash, gdb_val_opt));
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
            fmt::format(FMT_COMPILE("SELECT {}, {}, {}, {} FROM {}"),
                gdb_val_hash, gdb_val_opt, gdb_val_str, gdb_val_int, gdb_tbl_nodes));

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

void DbManager::wf_delete(const WfData& wf)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("DELETE FROM {} WHERE {} = ?"),
                gdb_tbl_wf, gdb_val_uuid));

        query.bindNoCopy(1, wf.uuid);
        query.exec();
        Log("Del workflow " << wf.name << "###" << wf.uuid << ", o=" << wf.open << ", fv=" << wf.fav << ", n=" << wf.nodes_inside);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite upd workflow " << wf.name << "###" << wf.uuid << ", " << LOG_EXP(ex));
    }
}

std::vector<WfData> DbManager::wf_load_meta_delete_empty()
{
    std::vector<WfData> wfs, del;
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {}, {}, {}, {}, {} FROM {}"),
                gdb_val_name, gdb_val_uuid, gdb_val_open, gdb_val_fav, gdb_val_nodes, gdb_tbl_wf));

        while(query.executeStep())
        {
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull())
                continue;
            
            WfData wfd;
            wfd.name = query.getColumn(0).getString();
            wfd.uuid = query.getColumn(1).getString();
            wfd.open = !query.getColumn(2).isNull() && query.getColumn(2).getInt() == 1;
            wfd.fav = !query.getColumn(3).isNull() && query.getColumn(3).getInt() == 1;
            wfd.nodes_inside = query.getColumn(4).getInt();

            if (wfd.nodes_inside > 0)
            {
                Log("Workflow loaded: " << wfd.name << "###" << wfd.uuid << ", o=" << wfd.open << ", fv=" << wfd.fav << ", n=" << wfd.nodes_inside);
                wfs.push_back(wfd);
            }
            else
            {
                del.push_back(wfd);
                Log("Workflow to delete: " << wfd.name << "###" << wfd.uuid << ", o=" << wfd.open << ", fv=" << wfd.fav);
            }            
        }
        Log("Loaded " << wfs.size() << " workflows");
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite load all workflows, " << LOG_EXP(ex));
    }

    try
    {
        if (!del.empty())
        {
            SQLite::Transaction transaction(*m_db);
            SQLite::Statement query(*m_db,
                fmt::format(FMT_COMPILE("DELETE FROM {} WHERE {} = ?"),
                    gdb_tbl_wf, gdb_val_uuid));
            for (auto& wfd : del)
            {
                query.bindNoCopy(1, wfd.uuid);
                query.exec();
                query.reset();
            }
            transaction.commit();
            Log("Deleted " << del.size() << " workflows");
        }
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite del empty workflows, " << LOG_EXP(ex));
    }

    return wfs;
}

void DbManager::wf_update_meta(const WfData& wf)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("INSERT OR REPLACE INTO {} ({}, {}, {}, {}, {}) VALUES (?, ?, ?, ?, ?)"),
                gdb_tbl_wf, gdb_val_uuid, gdb_val_name, gdb_val_open, gdb_val_fav, gdb_val_nodes));

        query.bindNoCopy(1, wf.uuid);
        query.bindNoCopy(2, wf.name);
        query.bind(3, wf.open);
        query.bind(4, wf.fav);
        query.bind(5, wf.nodes_inside);
        query.exec();
        Log("Upd workflow " << wf.name << "###" << wf.uuid << ", o=" << wf.open << ", fv=" << wf.fav << ", n=" << wf.nodes_inside);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite upd workflow " << wf.name << "###" << wf.uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::wf_load_json(WfData& wf)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {} FROM {} WHERE {}=?"),
                gdb_val_json, gdb_tbl_wf, gdb_val_uuid));

        query.bindNoCopy(1, wf.uuid);
        if (!query.executeStep())
            return;
        if (query.getColumn(0).isNull())
            return;
        wf.json = query.getColumn(0).getString();
        Log("Workflow loaded: " << wf.name << "###" << wf.uuid << ", j=" << wf.json.size() << ", n=" << wf.nodes_inside);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite load json workflow " << wf.name << "###" << wf.uuid << ", " << LOG_EXP(ex));
    }
}

void DbManager::wf_update_json(const WfData& wf)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("INSERT OR REPLACE INTO {} ({}, {}) VALUES (?, ?)"),
                gdb_tbl_wf, gdb_val_uuid, gdb_val_json));

        query.bindNoCopy(1, wf.uuid);
        query.bindNoCopy(2, wf.json);
        query.exec();
        Log("Workflow upd: " << wf.name << "###" << wf.uuid << ", j=" << wf.json.size() << ", n=" << wf.nodes_inside);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::SQLite, "SQLite upd json workflow " << wf.name << "###" << wf.uuid << ", " << LOG_EXP(ex));
    }
}

} // namespace mngr
