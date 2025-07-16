#include "client/client.hpp"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <iostream>



namespace irc
{
	// Constructor for the Client class
	// Manages a connectio to the server and handles sending and receiving messages
	Client::Client(boost::asio::io_context& io_context, const std::string& host, unsigned short port)
		: io_context_(io_context), // Keep a reference to the io_context for asynchronous operations
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
		boost::asio::async_write(socket_, boost::asio::buffer(message + "\r\n"),
			[this](const boost::system::error_code& error, std::size_t /*bytes*/)
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
			[this, buffer](const boost::system::error_code& error, std::size_t bytes) // Specificed Boost.Asio function signature.
			{
				if (!error)
				{
					std::string message(buffer->data(), bytes);
					std::cout << message << std::flush;
					startRead(); // Continue reading from the socket
				}
				else
				{
					logger_->error("Read error: {}", error.message());
				}
			});
	}

}