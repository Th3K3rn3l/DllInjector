#include "routes.h"
#include <sodium.h>
#include <iostream>
#include <jwt-cpp/jwt.h>
#include "../config.h"
#include <optional>
#include "../totp_util.h"
#include <random>


struct AuthInfo {
    int user_id;
    std::string username;
    bool licence_active; // для информации, можно использовать на защищённых путях
};

std::optional<AuthInfo> verify_jwt(const crow::request& req)
{
    auto auth = req.get_header_value("Authorization");
    if (auth.empty() || auth.substr(0, 7) != "Bearer ")
        return std::nullopt;

    std::string token = auth.substr(7);

    try {
        auto decoded = jwt::decode(token);

        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{ JWT_SECRET })
            .with_issuer("dllinjector")
            .verify(decoded);

        AuthInfo info;
        info.user_id = std::stoi(decoded.get_payload_claim("user_id").as_string());
        info.username = decoded.get_payload_claim("username").as_string();
        info.licence_active = decoded.get_payload_claim("licence").as_string() == "true";

        return info;
    }
    catch (...) {
        return std::nullopt;
    }
}

void register_routes(crow::SimpleApp& app, decltype(std::declval<DatabaseManager>().initialize(""))& db)
{
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
                if (!existing.empty())
                    return crow::response(400, "User already exists");

                char hashed_password[crypto_pwhash_STRBYTES];
                if (crypto_pwhash_str(
                    hashed_password,
                    pass.c_str(),
                    pass.length(),
                    crypto_pwhash_OPSLIMIT_INTERACTIVE,
                    crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
                {
                    return crow::response(500, "Hashing failed");
                }

                User newUser{ -1, user, std::string(hashed_password), false };
                db.insert(newUser);

                return crow::response(200, "Registration successful");
            });

    // --- ЛОГИН ---
    CROW_ROUTE(app, "/login").methods("POST"_method)
        ([&db](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x || !x.has("username") || !x.has("password"))
            return crow::response(400, "Missing credentials");

        std::string user = x["username"].s();
        std::string pass = x["password"].s();

        auto users = db.get_all<User>(sql::where(sql::c(&User::username) == user));
        if (users.empty()) return crow::response(404, "User not found");
        auto& u = users[0];

        if (crypto_pwhash_str_verify(u.password_hash.c_str(), pass.c_str(), pass.length()) != 0)
            return crow::response(401, "Password incorrect");

        crow::json::wvalue res;
        if (u.totp_enabled) {
            // Требуем 2FA – не выдаём токен, а возвращаем special статус и temp token
            // Создаём короткоживущий одноразовый токен для подтверждения 2FA
            std::string temp_token = jwt::create()
                .set_issuer("dllinjector")
                .set_payload_claim("user_id", jwt::claim(std::to_string(u.id)))
                .set_payload_claim("username", jwt::claim(user))
                .set_payload_claim("purpose", jwt::claim(std::string("2fa_login")))
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes(2))
                .sign(jwt::algorithm::hs256{ JWT_SECRET });
            res["requires_2fa"] = true;
            res["temp_token"] = temp_token;
            return crow::response(200, res);
        }

        // 2FA не включена – выдаём полноценный токен
        auto token = jwt::create()
            .set_issuer("dllinjector")
            .set_subject("auth")
            .set_type("JWS")
            .set_payload_claim("user_id", jwt::claim(std::to_string(u.id)))
            .set_payload_claim("username", jwt::claim(user))
            .set_payload_claim("licence", jwt::claim(u.licence_active ? std::string("true") : std::string("false")))
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes(5))
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        res["token"] = token;
        res["requires_2fa"] = false;
        return crow::response(200, res);
            });
    // --- СКАЧАТЬ DLL ---
    CROW_ROUTE(app, "/downloadDll")
        .methods("GET"_method)
        ([&db](const crow::request& req)
            {
                auto auth = verify_jwt(req);
                if (!auth)
                    return crow::response(401, "Unauthorized");

                // Лицензию проверяем только здесь
                if (!auth->licence_active)
                    return crow::response(403, "Licence inactive");

                auto users = db.get_all<User>(sql::where(sql::c(&User::id) == auth->user_id));
                if (users.empty())
                    return crow::response(404, "User not found");

                crow::response res;
                std::ifstream dll_file("dlls/myInjector.dll", std::ios::binary);
                if (!dll_file.is_open())
                    return crow::response(500, "DLL not found");

                std::vector<char> buffer((std::istreambuf_iterator<char>(dll_file)),
                    std::istreambuf_iterator<char>());

                res.code = 200;
                res.set_header("Content-Type", "application/octet-stream");
                res.set_header("Content-Disposition", "attachment; filename=\"myInjector.dll\"");
                res.body.assign(buffer.begin(), buffer.end());

                return res;
            });
    CROW_ROUTE(app, "/enable2fa").methods("POST"_method)
        ([&db](const crow::request& req) {
        auto auth = verify_jwt(req);
        if (!auth) return crow::response(401, "Unauthorized");

        // Генерируем секрет
        std::string secret = totp::generate_secret();
        // Пока не сохраняем в БД, вернём клиенту вместе с URI
        std::string uri = totp::make_uri(auth->username, secret, "DLLInjector");

        crow::json::wvalue res;
        res["secret"] = secret;
        res["uri"] = uri;
        return crow::response(200, res);
            });
    CROW_ROUTE(app, "/confirm2fa").methods("POST"_method)
        ([&db](const crow::request& req) {
        auto auth = verify_jwt(req);
        if (!auth) return crow::response(401, "Unauthorized");

        auto x = crow::json::load(req.body);
        if (!x || !x.has("secret") || !x.has("code"))
            return crow::response(400, "Missing fields");

        std::string secret = x["secret"].s();
        std::string code = x["code"].s();

        if (!totp::verify(secret, code))
            return crow::response(400, "Invalid code");

        // Сохраняем секрет и активируем
        auto users = db.get_all<User>(sql::where(sql::c(&User::id) == auth->user_id));
        if (users.empty()) return crow::response(404, "User not found");

        User u = users[0];
        u.totp_secret = secret;
        u.totp_enabled = true;
        db.update(u);

        return crow::response(200, "2FA enabled");
            });
    CROW_ROUTE(app, "/disable2fa").methods("POST"_method)
        ([&db](const crow::request& req) {
        auto auth = verify_jwt(req);
        if (!auth) return crow::response(401, "Unauthorized");

        auto x = crow::json::load(req.body);
        if (!x || !x.has("code"))
            return crow::response(400, "Missing code");

        std::string code = x["code"].s();
        auto users = db.get_all<User>(sql::where(sql::c(&User::id) == auth->user_id));
        if (users.empty()) return crow::response(404, "User not found");

        User& u = users[0];
        if (!u.totp_enabled) return crow::response(400, "2FA not enabled");

        if (!totp::verify(u.totp_secret, code))
            return crow::response(400, "Invalid code");

        u.totp_secret = "";
        u.totp_enabled = false;
        db.update(u);

        return crow::response(200, "2FA disabled");
            });
    CROW_ROUTE(app, "/verify-2fa-login").methods("POST"_method)
        ([&db](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x || !x.has("temp_token") || !x.has("code"))
            return crow::response(400, "Missing fields");

        std::string temp_token_str = x["temp_token"].s();
        std::string code = x["code"].s();

        // Проверяем temp_token
        std::optional<AuthInfo> temp_info;
        try {
            auto decoded = jwt::decode(temp_token_str);
            jwt::verify().allow_algorithm(jwt::algorithm::hs256{ JWT_SECRET }).verify(decoded);
            auto purpose = decoded.get_payload_claim("purpose").as_string();
            if (purpose != "2fa_login")
                return crow::response(400, "Invalid token purpose");
            AuthInfo info;
            info.user_id = std::stoi(decoded.get_payload_claim("user_id").as_string());
            info.username = decoded.get_payload_claim("username").as_string();
            temp_info = info;
        }
        catch (...) {
            return crow::response(401, "Invalid or expired temp token");
        }

        auto users = db.get_all<User>(sql::where(sql::c(&User::id) == temp_info->user_id));
        if (users.empty()) return crow::response(404, "User not found");
        User& u = users[0];

        if (!u.totp_enabled) return crow::response(400, "2FA not enabled for this user");

        if (!totp::verify(u.totp_secret, code))
            return crow::response(401, "Invalid 2FA code");

        // Успех – выдаём постоянный JWT
        auto token = jwt::create()
            .set_issuer("dllinjector")
            .set_subject("auth")
            .set_type("JWS")
            .set_payload_claim("user_id", jwt::claim(std::to_string(u.id)))
            .set_payload_claim("username", jwt::claim(u.username))
            .set_payload_claim("licence", jwt::claim(u.licence_active ? std::string("true") : std::string("false")))
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes(5))
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        crow::json::wvalue res;
        res["token"] = token;
        return crow::response(200, res);
            });
    }