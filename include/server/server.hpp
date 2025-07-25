#pragma once
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <optional>


namespace irc
{
	class Server
	{
	public:
		Server(boost::asio::io_context& io_context, unsigned short port);
		void start();

	
	private:
		void startAccept();
		void handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
		void broadcast(const std::string& message, const std::string& channel, std::shared_ptr<boost::asio::ip::tcp::socket> sender = nullptr);
		void sendToClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket, const std::string& message);
		void removeClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket, const std::string& quit_message = "");
		std::optional<std::string> parseCommand(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket);


		boost::asio::io_context& io_context_;
		boost::asio::ip::tcp::acceptor acceptor_;
		std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> clients_;
		std::map<std::shared_ptr<boost::asio::ip::tcp::socket>, std::string> nicknames_;
		std::map<std::string, std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>>> channels_;
		std::shared_ptr<spdlog::logger> logger_;
	};
} // namespace irc