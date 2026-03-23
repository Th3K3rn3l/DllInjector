#include <iostream>
#include <crow.h>
#include <sodium.h>
#include "database.h"

int main()
{
    if (sodium_init() < 0) return 1;

    const std::string DATABASE_PATH = "server_database.db";
    DatabaseManager database;
    auto db = database.initialize(DATABASE_PATH);
    db.sync_schema();

    crow::SimpleApp app;

    // --- РЕГИСТРАЦИЯ ---
    CROW_ROUTE(app, "/register").methods("POST"_method)
        ([&db](const crow::request& req)
            {
        auto x = crow::json::load(req.body);
        if (!x || !x.has("username") || !x.has("password"))
            return crow::response(400, "Invalid fields");

        std::string user = x["username"].s();
        std::string pass = x["password"].s();

        auto existing = db.get_all<User>(sql::where(sql::c(&User::username) == user));
        if (!existing.empty()) return crow::response(400, "User already exists");

        char hashed_password[crypto_pwhash_STRBYTES];
        if (crypto_pwhash_str(hashed_password, pass.c_str(), pass.length(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
            return crow::response(500, "Hashing failed");
        }

        User newUser{ -1, user, std::string(hashed_password), "", false };
        db.insert(newUser);
        return crow::response(200, "Registration successful");
            }
            );

    // --- ЛОГИН ---
    CROW_ROUTE(app, "/login").methods("POST"_method)
        ([&db](const crow::request& req)
            {
        auto x = crow::json::load(req.body);
        if (!x || !x.has("username") || !x.has("password"))
            return crow::response(400, "Missing credentials");

        std::string user = x["username"].s();
        std::string pass = x["password"].s();

        // Если "hwid" есть, берем его значение, если нет — записываем пустую строку
        std::string incoming_hwid = x.has("hwid") ? x["hwid"].s() : std::string("");
        std::cout << "Request from HWID: " << incoming_hwid << std::endl;
        auto matchingUsers = db.get_all<User>(sql::where(sql::c(&User::username) == user));
        if (matchingUsers.empty()) return crow::response(404, "User not found");

        auto& u = matchingUsers[0];

        // Проверка пароля
        if (crypto_pwhash_str_verify(u.password_hash.c_str(), pass.c_str(), pass.length()) != 0) {
            return crow::response(401, "Password incorrect");
        }

        // Подготовка ответа
        crow::json::wvalue res;
        res["licence_active"] = u.licence_active;
        res["can_inject"] = false; // По умолчанию инъекция запрещена
        res["decryption_key"] = ""; // Ключ пустой

        // ЛОГИКА ПРОВЕРКИ ЖЕЛЕЗА (только если есть лицензия)
        if (u.licence_active) {
            if (!incoming_hwid.empty()) {
                // Если HWID еще не привязан - привязываем
                if (u.hwid.empty()) {
                    u.hwid = incoming_hwid;
                    db.update(u);
                }

                // Сверяем HWID
                if (u.hwid == incoming_hwid) {
                    res["can_inject"] = true;
                    // В будущем здесь будет генерация реального ключа
                    res["decryption_key"] = "SUPER_SECRET_KEY_1337";
                }
                else {
                    return crow::response(403, "Hardware mismatch");
                }
            }
            else {
                // Вход без HWID (вероятно, сайт)
                res["info"] = "Logged in via web. App features disabled.";
            }
        }
        std::cout << "Successful login from " << req.remote_ip_address << std::endl;
        return crow::response(200, res);
            }
            );

    app.port(8000).run();
    return 0;
}