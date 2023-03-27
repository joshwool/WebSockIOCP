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

	void Operation(char *operation);
private:
	sqlite3 *m_db;
};

#endif // WEBSOCKIOCP_DATABASE_HPP
