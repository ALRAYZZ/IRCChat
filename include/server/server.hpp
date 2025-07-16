#pragma once
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <vector>
#include <string>


namespace irc
{
	class Server
	{
	public:
		Server(boost::asio::io_context& io_context, unsigned short port);
		void start();

	
	private:
		void startAccept();
		void broadcast(const std::string& message);

		boost::asio::io_context& io_context_;
		boost::asio::ip::tcp::acceptor acceptor_;
		std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> clients_;
		std::shared_ptr<spdlog::logger> logger_;
	};
} // namespace irc