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
		socket->async_read_some(boost::asio::buffer(*buffer),
			[this, socket, buffer](const boost::system::error_code& error, std::size_t bytes)
			{
				if (!error)
				{
					std::string message(buffer->data(), bytes);
					// Make a copy of the nickname to avoid iterator invalidation
					std::string client_nickname;
					if (nicknames_.find(socket) != nicknames_.end()) {
						client_nickname = nicknames_[socket];
					} else {
						client_nickname = "Unknown";
					}
					
					logger_->info("Received from {}: {}", client_nickname, message);
					parseCommand(message, socket);
					handleClient(socket); // Continue reading from the client
				}
				else
				{
					std::string client_nickname;
					if (nicknames_.find(socket) != nicknames_.end()) {
						client_nickname = nicknames_[socket];
					} else {
						client_nickname = "Unknown";
					}
					logger_->error("Read error from {}: {}", client_nickname, error.message());
					removeClient(socket, "Connection lost");
				}
			});
	}

	void Server::broadcast(const std::string& message, const std::string& channel, std::shared_ptr<boost::asio::ip::tcp::socket> sender)
	{
		if (channels_.find(channel) == channels_.end()) {
			logger_->warn("Attempted to broadcast to non-existent channel: {}", channel);
			return;
		}

		// Copy the sender nickname to avoid iterator invalidation
		std::string sender_nick = "server";
		if (sender && nicknames_.find(sender) != nicknames_.end()) {
			sender_nick = nicknames_[sender];
		}
		
		auto msg_ptr = std::make_shared<std::string>("<" + sender_nick + "> " + message + "\r\n");
		
		auto& members = channels_[channel];
		for (auto& client : members)
		{
			if (client != sender) // Don't send the message back to the sender
			{
				// Capture full_message and sender_nick by value to avoid iterator invalidation
				boost::asio::async_write(*client, boost::asio::buffer(*msg_ptr),
					[this, msg_ptr, sender_nick](const boost::system::error_code& error, std::size_t /*bytes*/)
					{
						if (error)
						{
							logger_->error("Write error to {}: {}", sender_nick, error.message());
						}
					});
			}
		}
	}

	void Server::sendToClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket, const std::string& message)
	{
		// Create a shared pointer to the message to ensure it stays alive
		auto msg_ptr = std::make_shared<std::string>("<server> " + message + "\r\n");
		boost::asio::async_write(*socket, boost::asio::buffer(*msg_ptr),
			[this, msg_ptr](const boost::system::error_code& error, std::size_t /*bytes*/)
			{
				if (error)
				{
					logger_->error("Write error: {}", error.message());
				}
			});
	}

	void Server::removeClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket, const std::string& quit_message)
	{
		// Copy the nickname before removing it
		std::string nickname = "Unknown";
		if (nicknames_.find(socket) != nicknames_.end()) {
			nickname = nicknames_[socket];
		}
		
		// Remove from clients list
		clients_.erase(std::remove(clients_.begin(), clients_.end(), socket), clients_.end());
		
		// Remove from all channels and broadcast quit message
		for (auto it = channels_.begin(); it != channels_.end();)
		{
			auto& [channel, members] = *it;
			auto member_it = std::find(members.begin(), members.end(), socket);
			if (member_it != members.end())
			{
				members.erase(member_it);
				broadcast(nickname + " has quit: " + quit_message, channel, nullptr);
			}
			
			if (members.empty())
			{
				it = channels_.erase(it);
			}
			else
			{
				++it;
			}
		}
		
		// Remove nickname mapping
		nicknames_.erase(socket);
		logger_->info("Client {} disconnected: {}", nickname, quit_message);
	}

	std::optional<std::string> Server::parseCommand(const std::string& message, std::shared_ptr<boost::asio::ip::tcp::socket> socket)
	{
		std::string trimmed = message;
		// Remove \r\n from the end
		while (!trimmed.empty() && (trimmed.back() == '\r' || trimmed.back() == '\n'))
		{
			trimmed.pop_back();
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
				std::string old_nick = nicknames_[socket];
				nicknames_[socket] = nickname;
				logger_->info("Nickname changed from {} to {}", old_nick, nickname);
				sendToClient(socket, "Nickname changed to " + nickname);
			}
		}
		else if (command == "JOIN")
		{
			std::string channel;
			iss >> channel;
			if (!channel.empty() && channel[0] == '#')
			{
				if (std::find(channels_[channel].begin(), channels_[channel].end(), socket) == channels_[channel].end())
				{
					channels_[channel].push_back(socket);
					std::string client_nick = nicknames_.find(socket) != nicknames_.end() ? nicknames_[socket] : "Unknown";
					logger_->info("{} joined {}", client_nick, channel);
					sendToClient(socket, "You joined " + channel);
					broadcast(client_nick + " joined " + channel, channel, nullptr);
				}
				else
				{
					sendToClient(socket, "You are already in " + channel);
				}
			}
		}
		else if (command == "PRIVMSG")
		{
			std::string channel;
			iss >> channel;
			
			// Check if user is in the channel
			if (channels_.find(channel) == channels_.end() || 
				std::find(channels_[channel].begin(), channels_[channel].end(), socket) == channels_[channel].end())
			{
				sendToClient(socket, "You must join " + channel + " to send messages");
				return std::nullopt;
			}

			// Get the rest of the message
			std::string msg;
			std::getline(iss, msg);
			msg.erase(0, msg.find_first_not_of(" \t"));
			
			if (!msg.empty())
			{
				std::string client_nick = nicknames_.find(socket) != nicknames_.end() ? nicknames_[socket] : "Unknown";
				logger_->info("Broadcasting message from {} in {}: {}", client_nick, channel, msg);
				broadcast(msg, channel, socket);
			}
		}
		else if (command == "QUIT")
		{
			std::string quit_message;
			std::getline(iss, quit_message);
			quit_message.erase(0, quit_message.find_first_not_of(" \t"));
			if (quit_message.empty())
			{
				quit_message = "No reason given";
			}
			removeClient(socket, quit_message);
		}
		else if (command == "WHO")
		{
			std::string channel;
			iss >> channel;
			
			if (channels_.find(channel) != channels_.end())
			{
				std::string user_list = "Users in " + channel + ": ";
				const auto& members = channels_[channel];
				
				if (members.empty())
				{
					user_list += "none";
				}
				else
				{
					for (size_t i = 0; i < members.size(); ++i)
					{
						// Check if the socket still exists in nicknames
						if (nicknames_.find(members[i]) != nicknames_.end())
						{
							user_list += nicknames_[members[i]];
							if (i < members.size() - 1)
							{
								user_list += ", ";
							}
						}
					}
				}
				sendToClient(socket, user_list);
				std::string client_nick = nicknames_.find(socket) != nicknames_.end() ? nicknames_[socket] : "Unknown";
				logger_->info("{} requested WHO in {}", client_nick, channel);
			}
			else
			{
				sendToClient(socket, "Channel " + channel + " does not exist");
			}
		}
		else
		{
			std::string client_nick = nicknames_.find(socket) != nicknames_.end() ? nicknames_[socket] : "Unknown";
			logger_->warn("Unknown command from {}: {}", client_nick, command);
		}
		
		return std::nullopt;
	}
} // namespace irc