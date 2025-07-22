#include "client/client_window.hpp"
#include <QApplication>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
	try
	{
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
		QApplication app(argc, argv);
		irc::ClientWindow window;
		window.show();
		return app.exec();
	}
	catch (const std::exception& e)
	{
		spdlog::error("Exception: {}", e.what());
		return 1;
	}
}