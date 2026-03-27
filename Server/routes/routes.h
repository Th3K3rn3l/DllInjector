#pragma once
#include <crow.h>
#include "../database.h"

void register_routes(crow::SimpleApp& app, decltype(std::declval<DatabaseManager>().initialize(""))& db);