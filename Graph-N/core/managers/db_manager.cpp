
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

inline constexpr auto gdb_tbl_err = "ERRORS";
inline constexpr auto gdb_tbl_opt = "SETTINGS";

inline constexpr auto gdb_opt_val_int = "value_int";
inline constexpr auto gdb_opt_val_str = "value_str";

bool DbManager::init()
{
    auto path = fs::get_root_directory_path();
    path /= APP_CAPTION_LOW ".db";

    try
    {
        m_db = std::make_unique<SQLite::Database>(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
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
				)", gdb_tbl_opt, gdb_val_id, gdb_val_name, gdb_opt_val_int, gdb_opt_val_str, gdb_val_when));

        //m_db->exec(R"(
        //		CREATE TABLE IF NOT EXISTS workflows (
        //			id INTEGER PRIMARY KEY AUTOINCREMENT,
        //			name TEXT NOT NULL,
        //			data TEXT NOT NULL,
        //			created DATETIME DEFAULT CURRENT_TIMESTAMP,
        //			updated DATETIME DEFAULT CURRENT_TIMESTAMP
        //		);
        //	)");

        //m_db->exec(R"(
  //          CREATE TABLE IF NOT EXISTS images (
  //              id INTEGER PRIMARY KEY,
  //              filename TEXT NOT NULL,
  //              workflow TEXT,
  //              prompt_pos TEXT,
        //		prompt_neg TEXT,
  //              seed INTEGER,
  //              favorite BOOLEAN DEFAULT 0,
  //              created DATETIME DEFAULT CURRENT_TIMESTAMP,
        //		updated DATETIME DEFAULT CURRENT_TIMESTAMP
  //          );
  //      )");
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite initialization error: " << ex.getErrorCode() << ", " << ex.getErrorStr() << ", " << ex.what());
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
        query.bind(1, file);
        query.bind(2, line);
        query.bind(3, cast::to_i32(category));
        query.bind(4, message);
        query.exec();
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite save err error: " << ex.getErrorCode() << ", " << ex.getErrorStr() << ", " << ex.what());
        return;
    }
}

void DbManager::set_option(const std::string& key, const std::string& value)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("INSERT OR REPLACE INTO {} ({}, {}) VALUES (?, ?)"),
                gdb_tbl_opt, gdb_val_name, gdb_opt_val_str));
        query.bind(1, key);
        query.bind(2, value);
        query.exec();
        Log("Opt updated: (" << key << "=" << value << ")");
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite save opt (" << key << "=" << value << ") error: " << ex.getErrorCode() << ", " << ex.getErrorStr() << ", " << ex.what());
        return;
    }
}

void DbManager::set_option(const std::string& key, int64_t value)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("INSERT OR REPLACE INTO {} ({}, {}) VALUES (?, ?)"),
                gdb_tbl_opt, gdb_val_name, gdb_opt_val_int));
        query.bind(1, key);
        query.bind(2, value);
        query.exec();
        Log("Opt updated: (" << key << "=" << value << ")");
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite save opt (" << key << "=" << value << ") error: " << ex.getErrorCode() << ", " << ex.getErrorStr() << ", " << ex.what());
        return;
    }
}

std::optional<std::string> DbManager::get_option_str(const std::string& key)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {} FROM {} WHERE {}=?"),
                gdb_opt_val_str, gdb_tbl_opt, gdb_val_name));
        query.bind(1, key);
        if (!query.executeStep())
            return std::nullopt;
        if (query.getColumn(0).isNull())
            return std::nullopt;
        return query.getColumn(0).getString();
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite load opt " << key << " error: " << ex.getErrorCode() << ", " << ex.getErrorStr() << ", " << ex.what());
    }
    return std::nullopt;
}

std::optional<int64_t> DbManager::get_option_int(const std::string& key)
{
    try
    {
        SQLite::Statement query(*m_db,
            fmt::format(FMT_COMPILE("SELECT {} FROM {} WHERE {}=?"),
                gdb_opt_val_int, gdb_tbl_opt, gdb_val_name));
        query.bind(1, key);
        if (!query.executeStep())
            return std::nullopt;
        if (query.getColumn(0).isNull())
            return std::nullopt;
        return query.getColumn(0).getInt64();
    }
    catch (const SQLite::Exception& ex)
    {
        LogExcep(ExcepCategory::sqlite, "SQLite load opt " << key << " error: " << ex.getErrorCode() << ", " << ex.getErrorStr() << ", " << ex.what());
    }
    return std::nullopt;
}

} // namespace mngr
