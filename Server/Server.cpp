#include <iostream>
#include <crow.h>

int main()
{
	crow::SimpleApp app;

	CROW_ROUTE(app, "/")([]() {
		return "Server is online";
		});

	app.port(8000).run();

	return 0;
}