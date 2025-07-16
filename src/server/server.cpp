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
		// Start accepting connections asynchronously
		acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& error) // Using the socket we start accepting connections
			{
				if (!error)
				{
					logger_->info("New client connected"); // WE use -> because logger is a shared pointer so to access its methods we need to dereference it
					clients_.push_back(socket); // Here we use . because its an actual obect, not a pointer
					// Start reading from the client
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
								logger_->info("Received message: {}", message);
								broadcast(message);

								// Second lambda call to create the infinite loop
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