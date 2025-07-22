#pragma once
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <string>
#include <QObject>


namespace irc
{
	class Client : public QObject 
	{
		Q_OBJECT
	public:
		Client(boost::asio::io_context& io_context, const std::string& host, unsigned short port, QObject* parent = nullptr);
		void start();
		void send(const std::string& message);

	signals:
		void messageReceived(const QString& message);
		void userListUpdated(const QStringList& users);

	private:
		void startRead();

		boost::asio::io_context& io_context_;
		boost::asio::ip::tcp::socket socket_;
		std::shared_ptr<spdlog::logger> logger_;
	};
} // namespace irc