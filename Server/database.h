#pragma once
#include <string>
#include <sqlite_orm/sqlite_orm.h>

namespace sql = sqlite_orm;

struct User
{
	int id;
	std::string username;
	std::string password_hash;
	std::string hwid;
	bool licence_active;
};


class DatabaseManager
{
public:
	DatabaseManager() {};
	~DatabaseManager() {};

	auto initialize(const std::string& path)
	{
		{
			return sql::make_storage(path,
				sql::make_table("users",
					sql::make_column("id", &User::id, sql::primary_key().autoincrement()),
					sql::make_column("username", &User::username, sql::unique()),
					sql::make_column("password_hash", &User::password_hash),
					sql::make_column("hwid", &User::hwid),
					sql::make_column("licence_active", &User::licence_active)
				)
			);
		}
	}
};

