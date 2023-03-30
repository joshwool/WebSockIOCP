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

	void Create(const char *query); // General function for Create statements

	bool Insert(const char *query, std::vector<std::string> values);

	// Separate function for SELECT (Read) statements as they require special callback
	std::string SelectString(const char *query, std::vector<std::string> values);
	std::vector<std::string> SelectMultiple(const char *query, std::vector<std::string> values, int columns);
	int SelectCount(const std::string& table, const std::string& columnName, const char *value);

	bool Update(const char *query, std::vector<std::string> values);
private:
	sqlite3 *m_db;
};

#endif // WEBSOCKIOCP_DATABASE_HPP
