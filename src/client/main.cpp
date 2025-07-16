#include "client/client.hpp"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <iostream>
#include <thread>


int main()
{
	try
	{
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
		boost::asio::io_context io_context;
		irc::Client client(io_context, "localhost", 6667);
		client.start();

		// Run io_context in a separate thread to allow asynchronous operations
		std::thread io_thread([&io_context]() {io_context.run(); });

		// Handle console input
		std::string message;
		while (std::getline(std::cin, message))
		{
			if (message == "/quit")
			{
				break;
			}
			client.send(message);
		}

		io_context.stop(); // Stop the io_context to exit the thread
		io_thread.join(); // Wait for the io_context thread to finish
	}
	catch (const std::exception& e)
	{
		spdlog::error("Exception: {}", e.what());
		return 1;
	}

	return 0;
}