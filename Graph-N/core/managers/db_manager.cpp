
#include "core/managers/db_manager.h"

namespace mngr
{

/*
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

inline constexpr auto gdb_val_id							= "id";
inline constexpr auto gdb_val_name							= "name";

inline constexpr auto gdb_tbl_opt							= "settings";

inline constexpr auto gdb_opt_val_int						= "value_int";
inline constexpr auto gdb_opt_val_str						= "value_str";

bool DbManager::init()
{
	auto path = fs::get_root_directory_path();
	path /= APP_CAPTION_LOW ".db";

	try
	{
		m_db = std::make_unique<SQLite::Database>(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
		m_db->exec("PRAGMA journal_mode = WAL;");
		m_db->exec("PRAGMA synchronous = NORMAL;");

		m_db->exec(tools::str::format(R"(
					CREATE TABLE IF NOT EXISTS %s (
						%s INTEGER PRIMARY KEY AUTOINCREMENT,
						%s TEXT NOT NULL UNIQUE,
						%s INTEGER,
						%s TEXT
					);
				)", gdb_tbl_opt, gdb_val_id, gdb_val_name, gdb_opt_val_int, gdb_opt_val_str));

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
	catch(const SQLite::Exception& ex)
	{
		LogExcep("SQLite initialization error: " << ex.getErrorCode() << ", " << ex.getErrorStr());
		return false;
	}

	Log("Database initialized: " << path);
	return true;
}

void DbManager::free()
{}

void DbManager::set_option(const std::string& key, const std::string& value)
{
	try 
	{
		SQLite::Statement query(*m_db, 
			tools::str::format("INSERT OR REPLACE INTO %s (%s, %s) VALUES (?, ?)", gdb_tbl_opt, gdb_val_name, gdb_opt_val_str));
		query.bind(1, key);
		query.bind(2, value);
		query.exec();
		Log("Opt updated: (" << key << "=" << value << ")");
	}
	catch (const SQLite::Exception& ex)
	{
		LogExcep("SQLite save opt (" << key << "=" << value << ") error: " << ex.getErrorCode() << ", " << ex.getErrorStr());
		return;
	}
}

void DbManager::set_option(const std::string& key, int64_t value)
{
	try
	{
		SQLite::Statement query(*m_db,
			tools::str::format("INSERT OR REPLACE INTO %s (%s, %s) VALUES (?, ?)", gdb_tbl_opt, gdb_val_name, gdb_opt_val_int));
		query.bind(1, key);
		query.bind(2, value);
		query.exec();
		Log("Opt updated: (" << key << "=" << value << ")");
	}
	catch (const SQLite::Exception& ex)
	{
		LogExcep("SQLite save opt (" << key << "=" << value << ") error: " << ex.getErrorCode() << ", " << ex.getErrorStr());
		return;
	}
}

std::optional<std::string> DbManager::get_option_str(const std::string& key)
{
	try
	{
		SQLite::Statement query(*m_db,
			tools::str::format("SELECT %s FROM %s WHERE %s=?", gdb_opt_val_str, gdb_tbl_opt, gdb_val_name));
		query.bind(1, key);
		if (!query.executeStep())
			return std::nullopt; 
		if (query.getColumn(0).isNull())
			return std::nullopt;
		return query.getColumn(0).getString();
	}
	catch (const SQLite::Exception& ex)
	{
		LogExcep("SQLite load opt " << key << " error: " << ex.getErrorCode() << ", " << ex.getErrorStr());
	}
	return std::nullopt;
}

std::optional<int64_t> DbManager::get_option_int(const std::string& key)
{
	try
	{
		SQLite::Statement query(*m_db,
			tools::str::format("SELECT %s FROM %s WHERE %s=?", gdb_opt_val_int, gdb_tbl_opt, gdb_val_name));
		query.bind(1, key);
		if (!query.executeStep())
			return std::nullopt;
		if (query.getColumn(0).isNull())
			return std::nullopt;
		return query.getColumn(0).getInt64();
	}
	catch (const SQLite::Exception& ex)
	{
		LogExcep("SQLite load opt " << key << " error: " << ex.getErrorCode() << ", " << ex.getErrorStr());
	}
	return std::nullopt;
}

} // namespace mngr