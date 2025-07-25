#include "client/client.hpp"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <iostream>
#include <QStringList>
#include <QString>
#include <QObject>
#include <sstream>
#include <array>
#include <memory>

namespace irc
{
	// Constructor for the Client class
	// Manages a connection to the server and handles sending and receiving messages
	Client::Client(boost::asio::io_context& io_context, const std::string& host, unsigned short port, QObject* parent)
		: QObject(parent),
		io_context_(io_context), // Keep a reference to the io_context for asynchronous operations
		socket_(io_context), // Create a TCP socket using the io_context
		logger_(spdlog::default_logger())
	{
		boost::asio::ip::tcp::resolver resolver(io_context); // This helps us resolve the host name to an IP address like localhost to a valid IP address
		auto endpoints = resolver.resolve(host, std::to_string(port)); // Resolve the host and port to a list of endpoints
		boost::asio::connect(socket_, endpoints); // Connect the socket to the first endpoint in the list
		logger_->info("Connected to server {}:{}", host, port);
	}

	void Client::start()
	{
		startRead();
	}

	// Send message to the server asynchronously
	void Client::send(const std::string& message)
	{
		// We call a boost asio function that sends data over a socket asynchronously
		// socket_ on this point holds the connection to the server since we connected to it in the constructor
		// We need to create a shared pointer to the message string so that it can be used in the callback handler else
		// it will be destroyed when the function returns and the callback will try to access a dangling pointer
		auto msg_ptr = std::make_shared<std::string>(message + "\r\n");
		boost::asio::async_write(socket_, boost::asio::buffer(*msg_ptr),
			[this, msg_ptr](const boost::system::error_code& error, std::size_t /*bytes*/)
			{
				if (error)
				{
					logger_->error("Write error: {}", error.message());
				}
			});
	}

	void Client::startRead()
	{
		// Buffer to hold the bytes read from the socket
		auto buffer = std::make_shared<std::array<char, 1024>>();
		// We start reading from the socket and storing the data in the buffer. We need to use *buffer deference because buffer is a shared pointer.
		socket_.async_read_some(boost::asio::buffer(*buffer),
			[this, buffer](const boost::system::error_code& error, std::size_t bytes) // Specified Boost.Asio function signature.
			{
				if (!error)
				{
					std::string message(buffer->data(), bytes);
					if (message.size() >= 19 && message.compare(0, 19, "<server> Users in ") == 0)
					{
						// Parse WHO response
						std::string user_list = message.substr(message.find(": ") + 2);
						if (user_list == "none")
						{
							emit userListUpdated(QStringList());
						}
						else
						{
							QStringList users;
							std::istringstream iss(user_list);
							std::string user;
							while (std::getline(iss, user, ','))
							{
								user.erase(0, user.find_first_not_of(" \t"));
								if (!user.empty())
								{
									users << QString::fromStdString(user);
								}
							}
							emit userListUpdated(users);
						}
					}
					else
					{
						emit messageReceived(QString::fromStdString(message));
					}
					startRead();
				}
				else
				{
					logger_->error("Read error: {}", error.message());
				}
			});
	}

}
