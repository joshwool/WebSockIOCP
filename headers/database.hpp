//
// Created by joshi on 27/03/2023.
//

#ifndef WEBSOCKIOCP_DATABASE_HPP
#define WEBSOCKIOCP_DATABASE_HPP

#include <core.hpp>

class Database {
public:
	Database(const char *dir);
	~Database();

	void Create(char *query); // General function for Create statements

	bool Insert(char *query, std::vector<std::string> values);

	int SelectCount(const std::string& table, const std::string& columnName, const char *value); // Separate function for SELECT (Read) statements as they require special callback
private:
	sqlite3 *m_db;
};

#endif // WEBSOCKIOCP_DATABASE_HPP
