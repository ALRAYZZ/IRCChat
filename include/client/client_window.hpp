#pragma once
#include "client/client.hpp"
#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>

namespace irc
{
	class ClientWindow : public QMainWindow
	{
		Q_OBJECT

public:
		ClientWindow(QWidget* parent = nullptr);


private slots:
		void sendMessage();
		void setNickname();
		void joinChannel();
		void refreshUserList();
		void onMessageReceived(const QString& message);
		void onUserListUpdated(const QStringList& users);


private:
		boost::asio::io_context io_context_;
		std::unique_ptr<Client> client_;
		QTextEdit* chatDisplay_;
		QLineEdit* messageInput_;
		QPushButton* sendButton_;
		QLineEdit* nicknameInput_;
		QPushButton* nicknameButton_;
		QLineEdit* channelInput_;
		QPushButton* joinButton_;
		QListWidget* userList_;
	};
}