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

	Create( // Create sessions table, if already exists this won't do anything
	  "CREATE TABLE IF NOT EXISTS sessions ("
	  "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
	  "session_id TEXT NOT NULL,"
	  "user_id INTEGER NOT NULL,"
	  "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,"
	  "FOREIGN KEY (user_id) REFERENCES users(id) );"
	  );

	Create( // Create user config table, if already exists this won't do anything
	  "CREATE TABLE IF NOT EXISTS user_config ("
	  "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
	  "user_id INTEGER NOT NULL,"
	  "stats TEXT NOT NULL,"
	  "test_config TEXT NOT NULL,"
	  "practice_config TEXT NOT NULL,"
	  "FOREIGN KEY (user_id) REFERENCES users(id) );"
	  );

	Create( // Creates tests table, if already exists this won't do anything
	  "CREATE TABLE IF NOT EXISTS tests ("
	  "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
	  "user_id INTEGER NOT NULL,"
	  "type TEXT NOT NULL,"
	  "number TEXT NOT NULL,"
	  "test_data TEXT NOT NULL,"
	  "date_completed TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,"
	  "FOREIGN KEY (user_id) REFERENCES users(id) );"
	  );

	Create( // Creates tests table, if already exists this won't do anything
	  "CREATE TABLE IF NOT EXISTS leaderboard ("
	  "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
	  "user_id INTEGER NOT NULL,"
	  "username TEXT NOT NULL,"
	  "type TEXT NOT NULL,"
	  "number TEXT NOT NULL,"
	  "test_data TEXT NOT NULL,"
	  "date_completed TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,"
	  "FOREIGN KEY (user_id) REFERENCES users(id) );"
	  );
}

Database::~Database() {
	sqlite3_close(m_db); // Closes database connection on deletion of object
}

void Database::Create(const char *query) {
	sqlite3_stmt *stmt; // Creates an SQL statement
	int result;

	result = sqlite3_prepare_v2( // Prepares statement with inputted query
	  m_db,
	  query,
	  -1, // Reads up to null terminator
	  &stmt,
	  nullptr);

	if (result != SQLITE_OK) {
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
		return;
	}

	result = sqlite3_step(stmt); // Steps through statement, i.e executes statement

	if (result != SQLITE_DONE) { // Checks if execution worked
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
	}

	sqlite3_finalize(stmt);
}

bool Database::Insert(const char *query, std::vector<std::string> values) {
	sqlite3_stmt *stmt;
	int result;

	result = sqlite3_prepare_v2(
	  m_db,
	  query,
	  -1,
	  &stmt,
	  nullptr);

	if (result != SQLITE_OK) {
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
		return false;
	}

	for (int i = 0; i < values.size(); i++) {
		sqlite3_bind_text( // Binds user inputted values to statement, using this method ensures that SQL injections cannot occur
		  stmt,
		  i+1,
		  values[i].c_str(),
		  -1,
		  SQLITE_TRANSIENT);
	}

	result = sqlite3_step(stmt);

	if (result != SQLITE_DONE) {
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
		return false;
	}

	sqlite3_finalize(stmt);

	return true;
}

std::string Database::SelectString(const char *query, std::vector<std::string> values) {
	sqlite3_stmt *stmt;
	int result;

	result = sqlite3_prepare_v2(
	  m_db,
	  query,
	  -1,
	  &stmt,
	  nullptr);

	if (result != SQLITE_OK) {
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
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
	std::string data;

	if (result == SQLITE_ROW) { // For the row found
		const unsigned char *data_cstr = sqlite3_column_text(stmt, 0);
		data = std::string(reinterpret_cast<const char*>(data_cstr)); // Copy data from row to data variable
	}
	else if (result == SQLITE_DONE){ // Could not find any row
		data = "null"; // Set data to null and handle later
	}
	else {
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
	}

	sqlite3_finalize(stmt);

	return data;
}

std::vector<std::string> Database::SelectMultiple(const char *query, std::vector<std::string> values, int columns) {
	sqlite3_stmt *stmt;
	int result;

	result = sqlite3_prepare_v2(
	  m_db,
	  query,
	  -1,
	  &stmt,
	  nullptr);

	if (result != SQLITE_OK) {
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
	}

	for (int i = 0; i < values.size(); i++) {
		sqlite3_bind_text(
		  stmt,
		  i+1,
		  values[i].c_str(),
		  -1,
		  SQLITE_TRANSIENT);
	}

	std::vector<std::string> results;

	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		for (int i = 0; i < columns; i++) { // For each column in row push value to results vector
			std::string data(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i)));
			results.push_back(data);
		}
	}
	if (result != SQLITE_DONE) {
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
	}

	sqlite3_finalize(stmt);

	return results;
}

int Database::SelectCount(const std::string& table, const std::string& columnName, const char *value) { // Counts the number of rows where condition is valid
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
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
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
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
	}

	sqlite3_finalize(stmt);

	return count;
}

bool Database::Update(const char *query, std::vector<std::string> values) {
	sqlite3_stmt *stmt;
	int result;

	result = sqlite3_prepare_v2(
	  m_db,
	  query,
	  -1,
	  &stmt,
	  nullptr);

	if (result != SQLITE_OK) {
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
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
		std::cerr << "SQLite operation failed: " << sqlite3_errmsg(m_db) << std::endl;
		return false;
	}

	sqlite3_finalize(stmt);

	return true;
}