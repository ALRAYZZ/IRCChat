#include "server/server.hpp"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

int main()
{
	try
	{
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
		boost::asio::io_context io_context;
		irc::Server server(io_context, 6667);
		server.start();
		io_context.run();
	}
	catch (const std::exception& e)
	{
		spdlog::error("Exception: {}", e.what());
		return 1;
	}


	return 0;
}