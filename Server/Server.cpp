#include <iostream>
#include <crow.h>
#include <sodium.h>
#include "database.h"
#include "routes/routes.h"

int main()
{
    if (sodium_init() < 0) return 1;

    const std::string DATABASE_PATH = "server_database.db";
    DatabaseManager database;
    auto db = database.initialize(DATABASE_PATH);
    db.sync_schema();

    crow::SimpleApp app;

    register_routes(app, db);

    app.port(8000).run();
    return 0;
}