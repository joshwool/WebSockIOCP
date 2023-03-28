#include <database.hpp>

Database::Database(const char *dir) {
	sqlite3_open(dir, &m_db); // Creates a database at the directory "dir"

	Create( // Create users table, if already exists this won't do anything
	  "CREATE TABLE IF NOT EXISTS users ("
	  "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
	  "username TEXT NOT NULL,"
	  "email TEXT NOT NULL,"
	  "pword_hash TEXT NOT NULL,"
	  "salt TEXT NOT NULL );"
	  );

	Create( // Create sessions table
	  "CREATE TABLE IF NOT EXISTS sessions ("
	  "id TEXT NOT NULL,"
	  "user_id INTEGER NOT NULL,"
	  "data TEXT,"
	  "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,"
	  "PRIMARY KEY(id),"
	  "FOREIGN KEY (user_id) REFERENCES users(id) );"
	  );
}

Database::~Database() {
	sqlite3_close(m_db);
}

void Database::Create(char *query) {
	sqlite3_stmt *stmt;
	int result;

	result = sqlite3_prepare_v2(
	  m_db,
	  query,
	  -1, // Reads up to null terminator
	  &stmt,
	  nullptr);

	if (result != SQLITE_OK) {
		std::cout << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
		return;
	}

	result = sqlite3_step(stmt);

	if (result != SQLITE_DONE) {
		std::cout << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
	}

	sqlite3_finalize(stmt);
}

bool Database::Insert(char *query, std::vector<std::string> values) {
	sqlite3_stmt *stmt;
	int result;

	result = sqlite3_prepare_v2(
	  m_db,
	  query,
	  -1,
	  &stmt,
	  nullptr);

	if (result != SQLITE_OK) {
		std::cout << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
		return false;
	}

	for (int i = 0; i < values.size(); i++) {
		sqlite3_bind_text(
		  stmt,
		  i+1,
		  values[i].c_str(),
		  -1,
		  SQLITE_TRANSIENT);
	}

	result = sqlite3_step(stmt);

	if (result != SQLITE_DONE) {
		std::cout << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
		return false;
	}

	sqlite3_finalize(stmt);

	return true;
}

int Database::SelectCount(const std::string& table, const std::string& columnName, const char *value) {
	sqlite3_stmt *stmt;
	int result;

	std::string insert_user_sql = "SELECT COUNT(*) FROM " + table + " WHERE " + columnName + " = ?";

	result = sqlite3_prepare_v2(
	  m_db,
	  insert_user_sql.c_str(),
	  -1, // Reads up to null terminator
	  &stmt,
	  nullptr);

	if (result != SQLITE_OK) {
		std::cout << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
		return -1;
	}

	sqlite3_bind_text( // Safely adds user inputted value into SQL statement, prevents SQL injection
	  stmt,
	  1,
	  value,
	  -1,
	  SQLITE_TRANSIENT);

	result = sqlite3_step(stmt);
	int count = -1;

	if (result == SQLITE_ROW) { // A row has been retrieved
		count = sqlite3_column_int(stmt, 0);
	}
	else {
		std::cout << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
	}

	sqlite3_finalize(stmt);

	return count;
}