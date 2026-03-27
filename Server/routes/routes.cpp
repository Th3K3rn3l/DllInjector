#include "routes.h"
#include <sodium.h>
#include <iostream>

void register_routes(crow::SimpleApp& app, decltype(std::declval<DatabaseManager>().initialize(""))& db)
{
    // --- –≈√»—“–¿÷»ﬂ ---
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
            });

    // --- ÀŒ√»Õ ---
    CROW_ROUTE(app, "/login").methods("POST"_method)
        ([&db](const crow::request& req)
            {
                auto x = crow::json::load(req.body);
                if (!x || !x.has("username") || !x.has("password"))
                    return crow::response(400, "Missing credentials");

                std::string user = x["username"].s();
                std::string pass = x["password"].s();

                std::string incoming_hwid = x.has("hwid") ? x["hwid"].s() : std::string("");
                std::cout << "Request from HWID: " << incoming_hwid << std::endl;

                auto matchingUsers = db.get_all<User>(sql::where(sql::c(&User::username) == user));
                if (matchingUsers.empty()) return crow::response(404, "User not found");

                auto& u = matchingUsers[0];

                if (crypto_pwhash_str_verify(u.password_hash.c_str(), pass.c_str(), pass.length()) != 0) {
                    return crow::response(401, "Password incorrect");
                }

                crow::json::wvalue res;
                res["licence_active"] = u.licence_active;
                res["can_inject"] = false;
                res["decryption_key"] = "";

                if (u.licence_active) {
                    if (!incoming_hwid.empty()) {
                        if (u.hwid.empty()) {
                            u.hwid = incoming_hwid;
                            db.update(u);
                        }

                        if (u.hwid == incoming_hwid) {
                            res["can_inject"] = true;
                            res["decryption_key"] = "SUPER_SECRET_KEY_1337";
                        }
                        else {
                            return crow::response(403, "Hardware mismatch");
                        }
                    }
                    else {
                        res["info"] = "Logged in via web. App features disabled.";
                    }
                }

                std::cout << "Successful login from " << req.remote_ip_address << std::endl;
                return crow::response(200, res);
            });
}