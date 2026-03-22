#include <iostream>
#include <crow.h> // mini web framework

#include "database.h"

int main()
{
	// Setting up database
	const std::string DATABASE_PATH = "server_database.db";
	DatabaseManager database;
	auto db = database.initialize(DATABASE_PATH);
	db.sync_schema();


	crow::SimpleApp app;

	// Routes
	CROW_ROUTE(app, "/register").methods("POST"_method)
        ([&db](const crow::request& req)
            {
                auto x = crow::json::load(req.body);
                if (!x) return crow::response(400, "Invalid request");

                if (!x.has("username") || !x.has("password") || !x.has("hwid"))
                    return crow::response(400, "Not enough fields");

                std::string user = x["username"].s();
                std::string pass = x["password"].s(); // В идеале тут должен быть хеш
                std::string hwid = x["hwid"].s();
                // Проверяем, не занят ли логин
                auto existing = db.get_all<User>(sql::where(sql::c(&User::username) == user));
                if (!existing.empty()) return crow::response(400, "User already exists");

                // Создаем нового пользователя
                User newUser{ -1, user, pass, hwid, false }; // -1 для autoincrement ID
                try {
                    db.insert(newUser);
                    return crow::response(200, "Registration successful");
                }
                catch (...) {
                    return crow::response(500, "Database error");
                }
            }
        );
		
    CROW_ROUTE(app, "/login").methods("POST"_method)
        ([&db](const crow::request& req)
            {
                auto x = crow::json::load(req.body);
                if (!x) return crow::response(400, "Invalid request");

                if (!x.has("username") || !x.has("password") || !x.has("hwid"))
                    return crow::response(400, "Not enough fields");

                std::string user = x["username"].s();
                std::string pass = x["password"].s(); // В идеале тут должен быть хеш
                std::string hwid = x["hwid"].s();

                auto matchingUsers = db.get_all<User>
                    (sql::where(sql::c(&User::username) == user));
                if (matchingUsers.empty())
                    return crow::response(404, "You need to register first");
                auto& existingUser = matchingUsers[0];
                // проверка пароля
                if (existingUser.password_hash != pass)
                    return crow::response(401, "Password incorrect");
                // проверка железа
                if (existingUser.hwid != hwid)
                    return crow::response(403, "This account is linked to another device");
                // успешный вход
                return crow::response(200, "Login successful");
            }
        );


	app.port(8000).run();

	return 0;
}