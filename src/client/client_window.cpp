#include "client/client_window.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <thread>


namespace irc
{
	ClientWindow::ClientWindow(QWidget* parent)
		: QMainWindow(parent),
		client_(std::make_unique<Client>(io_context_, "localhost", 6667, this))
	{
		setWindowTitle("IRC Chat Client");
		resize(600, 400);

		// Create widgets
		chatDisplay_ = new QTextEdit(this);
		chatDisplay_->setReadOnly(true);
		messageInput_ = new QLineEdit(this);
		sendButton_ = new QPushButton("Send", this);
		nicknameInput_ = new QLineEdit(this);
		nicknameInput_->setPlaceholderText("Enter nickname");
		nicknameButton_ = new QPushButton("Set Nickname", this);
		channelInput_ = new QLineEdit(this);
		channelInput_->setPlaceholderText("Enter channel (e.g., #general)");
		joinButton_ = new QPushButton("Join Channel", this);
		userList_ = new QListWidget(this);
		QPushButton* refreshButton_ = new QPushButton("Refresh User List", this);

		// Layout
		QWidget* centralWidget = new QWidget(this);
		QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
		QHBoxLayout* inputLayout = new QHBoxLayout();
		QHBoxLayout* controlLayout = new QHBoxLayout();

		inputLayout->addWidget(messageInput_);
		inputLayout->addWidget(sendButton_);
		controlLayout->addWidget(nicknameInput_);
		controlLayout->addWidget(nicknameButton_);
		controlLayout->addWidget(channelInput_);
		controlLayout->addWidget(joinButton_);
		controlLayout->addWidget(refreshButton_);
		mainLayout->addWidget(chatDisplay_);
		mainLayout->addLayout(controlLayout);
		mainLayout->addLayout(inputLayout);
		mainLayout->addWidget(userList_);

		setCentralWidget(centralWidget);

		// Connect signals
		connect(sendButton_, &QPushButton::clicked, this, &ClientWindow::sendMessage);
		connect(messageInput_, &QLineEdit::returnPressed, this, &ClientWindow::sendMessage);
		connect(nicknameButton_, &QPushButton::clicked, this, &ClientWindow::setNickname);
		connect(joinButton_, &QPushButton::clicked, this, &ClientWindow::joinChannel);
		connect(refreshButton_, &QPushButton::clicked, this, &ClientWindow::refreshUserList);
		connect(client_.get(), &Client::messageReceived, this, &ClientWindow::onMessageReceived);
		connect(client_.get(), &Client::userListUpdated, this, &ClientWindow::onUserListUpdated);

		// Start networking
		client_->start();
		std::thread io_thread([this]() { io_context_.run(); });
		io_thread.detach();
	}

	void ClientWindow::sendMessage()
	{
		QString message = messageInput_->text().trimmed();
		if (!message.isEmpty())
		{
			QString channel = channelInput_->text().trimmed();
			if (!channel.isEmpty())
			{
				client_->send("PRIVMSG " + channel.toStdString() + " " + message.toStdString());
			}
			messageInput_->clear();
		}
	}

	void ClientWindow::setNickname()
	{
		QString nickname = nicknameInput_->text().trimmed();
		if (!nickname.isEmpty())
		{
			client_->send("NICK " + nickname.toStdString());
			nicknameInput_->clear();
		}
	}

	void ClientWindow::joinChannel()
	{
		QString channel = channelInput_->text().trimmed();
		if (!channel.isEmpty())
		{
			client_->send("JOIN " + channel.toStdString());
		}
	}

	void ClientWindow::refreshUserList()
	{
		QString channel = channelInput_->text().trimmed();
		if (!channel.isEmpty())
		{
			client_->send("WHO " + channel.toStdString());
		}
	}

	void ClientWindow::onMessageReceived(const QString& message)
	{
		chatDisplay_->append(message);
	}

	void ClientWindow::onUserListUpdated(const QStringList& users)
	{
		userList_->clear();
		userList_->addItems(users);
	}
} // namespace irc	
