
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
bool DbManager::init()
{
	auto path = fs::get_res_directory_path();
	path /= APP_CAPTION_LOW ".db";

	try
	{
		m_db = std::make_unique<SQLite::Database>(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
		m_db->exec("PRAGMA journal_mode = WAL;");
		m_db->exec("PRAGMA synchronous = NORMAL;");

		m_db->exec(R"(
				CREATE TABLE IF NOT EXISTS settings (
					id INTEGER PRIMARY KEY AUTOINCREMENT,
					name TEXT NOT NULL,
					value_str TEXT,
					value_int INTEGER,
				);
			)");

		m_db->exec(R"(
				CREATE TABLE IF NOT EXISTS workflows (
					id INTEGER PRIMARY KEY AUTOINCREMENT,
					name TEXT NOT NULL,
					data TEXT NOT NULL,
					created DATETIME DEFAULT CURRENT_TIMESTAMP,
					updated DATETIME DEFAULT CURRENT_TIMESTAMP
				);
			)");

		m_db->exec(R"(
            CREATE TABLE IF NOT EXISTS images (
                id INTEGER PRIMARY KEY,
                filename TEXT NOT NULL,
                workflow TEXT,
                prompt_pos TEXT,
				prompt_neg TEXT,
                seed INTEGER,
                favorite BOOLEAN DEFAULT 0,
                created DATETIME DEFAULT CURRENT_TIMESTAMP,
				updated DATETIME DEFAULT CURRENT_TIMESTAMP
            );
        )");
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

} // namespace mngr