#include "server/server.hpp"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <functional>
#include <sstream>

namespace irc 
{
	// Constructor for the Server class
	Server::Server(boost::asio::io_context& io_context, unsigned short port)
		: io_context_(io_context),
		acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
		logger_(spdlog::default_logger())
	{
		logger_->info("Server initialized on port {}", port);
	}


	void Server::start()
	{
		startAccept();
	}

	void Server::startAccept()
	{
		auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);
		acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& error)
			{
				if (!error)
				{
					logger_->info("New client connected");
					clients_.push_back(socket); // Add the new client socket to the list of clients
					nicknames_[socket] = "Anonymous"; // Assign a default nickname
					handleClient(socket);
				}
				else
				{
					logger_->error("Accept error: {}", error.message());
				}
				startAccept(); // Continue accepting new clients
			});
	}
	void Server::handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
	{
		auto buffer = std::make_shared<std::array<char, 1024>>();
		// After connection, we read data from the client and storing it in a buffer
					// Using callback handler which is giving a function to be called when the read operation is complete
					// The lambda function is a recursive call to read from the client again after processing the message
					// Create an infinite loop, since the function calls itself recursively
					// We are definning it twice because its a lambda
					// But its like saying:
					// void read()
					// {
					//		read();
					// }
					// The function will keep calling itself until an error occurs or the client disconnects

		socket->async_read_some(boost::asio::buffer(*buffer),
			[this, socket, buffer](const boost::system::error_code& error, std::size_t bytes)
			{
				if (!error)
				{
					std::string message(buffer->data(), bytes);
					logger_->info("Received from {}: {}", nicknames_[socket], message);
					auto broadcast_message = parseCommand(message, socket);
					if (broadcast_message)
					{
						broadcast(*broadcast_message, socket);
					}
					handleClient(socket); // Continue reading from the client
				}
				else
				{
					logger_->error("Read error from {}: {}", nicknames_[socket], error.message());
					clients_.erase(std::remove(clients_.begin(), clients_.end(), socket), clients_.end());
					nicknames_.erase(socket); // Remove the nickname associated with the socket
				}
			});
	}

	void Server::broadcast(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> sender)
	{
		std::string sender_nick = sender && nicknames_.count(sender) ? nicknames_[sender] : "server";
		std::string full_message = "<" + sender_nick + "> " + message;
		for (auto& client : clients_)
		{
			if (client != sender) // Dont send the message back to the sender
			{
				boost::asio::async_write(*client, boost::asio::buffer("<" + (sender ? nicknames_[sender] : "server") + "> " + message),
					[this](const boost::system::error_code& error, std::size_t/*bytes*/)
					{
						if (error)
						{
							logger_->error("Write error: {}", error.message());
						}
					});
			}
		}
	}

	std::optional<std::string> Server::parseCommand(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket)
	{
		std::string trimmed = message;
		if (trimmed.size() >= 2 && trimmed.compare(trimmed.size() - 2, 2, "\r\n") == 0)
		{
			trimmed = trimmed.substr(0, trimmed.size() - 2);
		}
		std::istringstream iss(trimmed);
		std::string command;
		iss >> command;

		if (command == "NICK")
		{
			std::string nickname;
			std::getline(iss, nickname);
			nickname.erase(0, nickname.find_first_not_of(" \t"));
			if (!nickname.empty())
			{
				nicknames_[socket] = nickname;
				logger_->info("Nickname set for client: {}", nickname);
				return "Nickname changed to " + nickname;
			}
			return std::nullopt; // No nickname provided
		}
		else if (command == "PRIVMSG")
		{
			std::string msg;
			std::getline(iss, msg);
			msg.erase(0, msg.find_first_not_of(" \t"));
			if (!msg.empty())
			{
				return msg;
			}
			return std::nullopt; // No message provided
		}
		return std::nullopt; // Unknown command or no message
	}
} // namespace irc