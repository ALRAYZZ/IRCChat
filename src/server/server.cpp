#include "server/server.hpp"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <functional>

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
		auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_); // Pointer to a new socket using smart pointer to control its lifetime
		// Lambda function to handle the accept operation
		acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& error) // Using the socket we start accepting connections
			{
				if (!error)
				{
					logger_->info("New client connected");
					clients_.push_back(socket);
					// Start reading from the client
					auto buffer = std::make_shared<std::array<char, 1024>>();
					socket->async_read_some(boost::asio::buffer(*buffer),
						[this, socket, buffer](const boost::system::error_code& error, std::size_t bytes)
						{
							if (!error)
							{
								std::string message(buffer->data(), bytes);
								logger_->info("Received message: {}", message);
								broadcast(message);
								// Continue reading from the client
								socket->async_read_some(boost::asio::buffer(*buffer),
									[this, socket, buffer](const boost::system::error_code& error, std::size_t bytes)
									{
										if (!error)
										{
											std::string message(buffer->data(), bytes);
											logger_->info("Received message: {}", message);
											broadcast(message);
										}
										else
										{
											logger_->error("Read error: {}", error.message());
											clients_.erase(std::remove(clients_.begin(), clients_.end(), socket), clients_.end());
										}
									});
							}
							else
							{
								logger_->error("Read error: {}", error.message());
								clients_.erase(std::remove(clients_.begin(), clients_.end(), socket), clients_.end());
							}
						});
				}
				else
				{
					logger_->error("Accept error: {}", error.message());
				}
				startAccept();
			});
	}

	void Server::broadcast(const std::string& message)
	{
		for (auto& client : clients_)
		{
			boost::asio::async_write(*client, boost::asio::buffer(message),
				[this](const boost::system::error_code& error, std::size_t /*bytes*/)
				{
					if (error)
					{
						logger_->error("Write error: {}", error.message());
					}
				});
		}
	}
} // namespace irc