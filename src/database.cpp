#include <database.hpp>

Database::Database(const char *dir) {
	sqlite3_open(dir, &m_db); // Creates a database at the directory "dir"

	Operation( // Create users table, if already exists this won't do anything
	  "CREATE TABLE users ("
	  "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
	  "username TEXT NOT NULL,"
	  "email TEXT NOT NULL,"
	  "pword_hash TEXT NOT NULL,"
	  "salt TEXT NOT NULL );"
	  );

	Operation( // Create sessions table
	  "CREATE TABLE sessions ("
	  "id TEXT NOT NULL,"
	  "user_id INTEGER NOT NULL,"
	  "data TEXT NOT NULL,"
	  "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,"
	  "PRIMARY KEY(id),"
	  "FOREIGN KEY (user_id) REFERENCES users(id) );"
	  );
}

Database::~Database() {
	sqlite3_close(m_db);
}

void Database::Operation(char *operation) {
	char *errmsg;

	int result = sqlite3_exec(
	  m_db,
	  operation,
	  nullptr,
	  nullptr,
	  &errmsg);

	if (result != SQLITE_OK) {
		std::cout << "SQLite operation failed: " << errmsg << std::endl;
	}
}