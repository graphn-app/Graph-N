
#include "core/managers/db_manager.h"

namespace mngr
{

/*
TODO - del

CREATE TABLE items (
    id INTEGER PRIMARY KEY,      -- int64 / uint64 (careful, will truncate to max signed - 9'223'372'036'854'775'807)
    name TEXT,                   -- std::string (any length)
    flags INTEGER,               -- uint32
    rating REAL,                 -- float / double
    preview_data BLOB,           -- std::vector<char> (image)
    is_favorite INTEGER          -- bool

save
---
query.bind(1, (int64_t)my_uint64_id); // Кастуем к signed, биты те же
query.bind(2, my_string);
query.bind(3, my_bool ? 1 : 0);

load
---
uint64_t id = (uint64_t)query.getColumn(0).getInt64(); // cast to unsigned if required
std::string s = query.getColumn(1).getText();
bool b = query.getColumn(2).getInt() > 0;
);
*/

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

inline constexpr auto gdb_tbl_err = "ERRORS";
inline constexpr auto gdb_tbl_opt = "SETTINGS";
inline constexpr auto gdb_tbl_nodes = "NODES";

#define LOG_VALUE(v)                   "(" << v.ival << ", " << v.sval << ")"
#define LOG_KEY(k)                     "opt: '" << k << "'"
#define LOG_KEY_VAL(k, v)              LOG_KEY(k) << " = " << LOG_VALUE(v)
#define LOG_NOPT_KEY(h, k)             "id=" << h << ", opt: '" << cast::to_i32(k)
#define LOG_NOPT_KEY_VAL(h, k, v)      LOG_NOPT_KEY(h, k) << "' = " << LOG_VALUE(v)
#define LOG_EXP(e)                     "err: (" << e.getErrorCode() << ", " << e.getErrorStr() << ", " << ex.what()

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
				)", gdb_tbl_nodes, gdb_val_id, gdb_val_hash, gdb_val_opt, gdb_val_int, gdb_val_str, gdb_val_when, gdb_val_hash, gdb_val_opt));
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite initialization " << LOG_EXP(ex));
        return false;
    }

    Log("Database initialized: " << path);
    return true;
}

void DbManager::save_exception(ExcepCategory category, const std::string& message, const char* file, int line)
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
        return;
    }
}

void DbManager::save_option(const std::string& key, const Value& value)
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
        return;
    }
}

std::vector<DbOption> DbManager::load_all_options()
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {}, {}, {} FROM {}"),
                gdb_val_name, gdb_val_str, gdb_val_int, gdb_tbl_opt));
        if (!query.executeStep())
            return {};

        std::vector<DbOption> ret;
        ret.reserve(100);
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

void DbManager::save_all_options(const std::vector<DbOption>& opts)
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

void DbManager::save_node_option(hid node_hash, NodeOption opt, const Value& value)
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
        return;
    }
}

std::optional<Value> DbManager::load_node_option(hid node_hash, NodeOption opt)
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

std::vector<std::pair<hid, Value>> DbManager::load_all_node_options(NodeOption opt)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {}, {}, {} FROM {} WHERE {}=? ORDER BY {}"),
                gdb_val_hash, gdb_val_str, gdb_val_int, gdb_tbl_nodes, gdb_val_opt, gdb_val_hash));
        query.bind(1, cast::to_i32(opt));

        std::vector<std::pair<hid, Value>> ret;
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

} // namespace mngr
