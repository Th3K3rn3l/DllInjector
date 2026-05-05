#pragma once
#include <string>
#include <sqlite_orm/sqlite_orm.h>

namespace sql = sqlite_orm;

struct User
{
	int id;
	std::string username;
	std::string password_hash;
	bool licence_active;
	std::string totp_secret;      // base32-encoded секрет (пусто, если 2FA не включена)
	bool totp_enabled;            // true, если 2FA активна
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
					sql::make_column("licence_active", &User::licence_active),
					sql::make_column("totp_secret", &User::totp_secret),
					sql::make_column("totp_enabled", &User::totp_enabled)
				)
			);
		}
	}
};

