
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
WORKFLOWS:    | id        | hash                | name                     | open          | favourite     | json           | at
______________|------------------------------------------------------------------------------------------------------------------------------------------------
              | int       | str                 | str                      | int           | int           | str            | datetime
---------------------------------------------------------------------------------------------------------------------------------------------------------------
              | prim      | rid as string       | name (caption)           | is open       | is favourited | json-view      | create/update time
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
                        {} DATETIME DEFAULT CURRENT_TIMESTAMP
					);
				)", gdb_tbl_wf, gdb_val_id, gdb_val_hash, gdb_val_name, gdb_val_open, gdb_val_fav, gdb_val_json, gdb_val_when));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite initialization " << LOG_EXP(ex));
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
        LogExcep(ExcepCategory::sqlite, "SQLite save " << LOG_EXP(ex));
    }
}

void DbManager::option_save(const std::string& key, const Value& value)
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
        LogExcep(ExcepCategory::sqlite, "SQLite save " << LOG_KEY_VAL(key, value) << ", " << LOG_EXP(ex));
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

            ret.push_back({});
            DbOption& opt = ret.back();
            opt.name = query.getColumn(0).getString();
            opt.val = { query.getColumn(1).getString(), query.getColumn(2).getInt64() };            
        }

        return ret;
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite load all options " << LOG_EXP(ex));
    }
    return {};
}

void DbManager::option_save_all(const std::vector<DbOption>& opts)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("INSERT OR REPLACE INTO {} ({}, {}, {}) VALUES (?, ?, ?)"),
                gdb_tbl_opt, gdb_val_name, gdb_val_int, gdb_val_str));

        SQLite::Transaction transaction(*m_db);
        for (auto& opt : opts)
        {
            query.bindNoCopy(1, opt.name);
            const Value& v = opt.val.value();
            query.bind(2, v.ival);
            query.bindNoCopy(3, v.sval);

            query.exec();
            query.reset();
        }
        transaction.commit();
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite save all options " << LOG_EXP(ex));
    }
}

void DbManager::node_option_save(hid node_hash, NodeOption opt, const Value& value)
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
        LogExcep(ExcepCategory::sqlite, "SQLite save " << LOG_NOPT_KEY_VAL(node_hash, opt, value) << ", " << LOG_EXP(ex));
    }
}

std::optional<Value> DbManager::node_option_load(hid node_hash, NodeOption opt)
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
        std::optional<Value> v;
        v.emplace(query.getColumn(0).getString(), query.getColumn(1).getInt64());
        return v;
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite load " << LOG_NOPT_KEY(node_hash, opt) << ", " << LOG_EXP(ex));
    }
    return std::nullopt;
}

std::vector<std::pair<hid, Value>> DbManager::node_option_load_all(NodeOption opt)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {}, {}, {} FROM {} WHERE {}=?"),
                gdb_val_hash, gdb_val_str, gdb_val_int, gdb_tbl_nodes, gdb_val_opt));
        query.bind(1, cast::to_i32(opt));

        std::vector<std::pair<hid, Value>> ret;
        ret.reserve(100);
        while(query.executeStep())
        {
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull() && query.getColumn(2).isNull())
                continue;

            const std::string hash = query.getColumn(0).getString();
            Value v(query.getColumn(1).getString(), query.getColumn(2).getInt64());
            ret.emplace_back(std::strtoull(hash.c_str(), nullptr, 10), std::move(v));
        }

        return ret;
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite load all " << LOG_NOPT_KEY(0, opt) << ", " << LOG_EXP(ex));
    }
    return {};
}

void DbManager::wf_delete(const WfData& /*wf*/)
{
    // TODO
}

std::vector<WfData> DbManager::wf_load_meta()
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {}, {}, {}, {} FROM {}"),
                gdb_val_name, gdb_val_hash, gdb_val_open, gdb_val_fav, gdb_tbl_wf));

        std::vector<WfData> wfs;
        while(query.executeStep())
        {
            if (query.getColumn(0).isNull() || query.getColumn(1).isNull())
                continue;

            wfs.push_back({});
            WfData& wf = wfs.back();
            wf.name = query.getColumn(0).getString();
            wf.id_str = query.getColumn(1).getString();
            wf.id = std::strtoull(wf.id_str.c_str(), nullptr, 10);
            wf.open = !query.getColumn(2).isNull() && query.getColumn(2).getInt() == 1;
            wf.fav = !query.getColumn(3).isNull() && query.getColumn(3).getInt() == 1;
        }

        return wfs;
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite load all workflows, " << LOG_EXP(ex));
    }
    return {};
}

void DbManager::wf_update_meta(const WfData& wf)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("INSERT OR REPLACE INTO {} ({}, {}, {}, {}) VALUES (?, ?, ?, ?)"),
                gdb_tbl_wf, gdb_val_hash, gdb_val_name, gdb_val_open, gdb_val_fav));

        query.bindNoCopy(1, wf.id_str);
        query.bindNoCopy(2, wf.name);
        query.bind(3, wf.open);
        query.bind(4, wf.fav);
        query.exec();
        Log("Upd workflow " << wf.name << "###" << wf.id);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite upd workflow " << wf.name << "###" << wf.id << ", " << LOG_EXP(ex));
    }
}

void DbManager::wf_load_json(WfData& wf)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {} FROM {} WHERE {}=?"),
                gdb_val_json, gdb_tbl_wf, gdb_val_hash));

        query.bindNoCopy(1, wf.id_str);
        if (!query.executeStep())
            return;
        if (query.getColumn(0).isNull())
            return;
        wf.json = query.getColumn(0).getString();
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite load json workflow " << wf.name << "###" << wf.id << ", " << LOG_EXP(ex));
    }
}

void DbManager::wf_update_json(const WfData& wf)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("INSERT OR REPLACE INTO {} ({}, {}) VALUES (?, ?)"),
                gdb_tbl_wf, gdb_val_hash, gdb_val_json));

        query.bindNoCopy(1, wf.id_str);
        query.bindNoCopy(2, wf.json);
        query.exec();
        Log("Upd json workflow " << wf.name << "###" << wf.id);
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite upd json workflow " << wf.name << "###" << wf.id << ", " << LOG_EXP(ex));
    }
}

} // namespace mngr
