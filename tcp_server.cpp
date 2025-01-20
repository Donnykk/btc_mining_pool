#include "tcp_server.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

Miner::Miner(const std::string &username, const std::string &password, const std::string &address)
{
    username_ = username;
    password_ = password;
    address_ = address;
}

std::string Miner::getUsername() const
{
    return username_;
}

std::string Miner::getAddress() const
{
    return address_;
}

bool Miner::verifyPwd(const std::string &password) const
{
    return password_ == password;
}

MinerManager::MinerManager(const std::string &dbHost, const std::string &dbUser, const std::string &dbPassword, const std::string &dbName)
    : db_(mysql_init(nullptr))
{
    if (!mysql_real_connect(db_, dbHost.c_str(), dbUser.c_str(), dbPassword.c_str(), dbName.c_str(), 3306, nullptr, 0))
    {
        std::cerr << "Failed to connect to MySQL: " << mysql_error(db_) << std::endl;
        exit(EXIT_FAILURE);
    }
}

bool MinerManager::initDatabase()
{
    const char *createTableSQL =
        "CREATE TABLE IF NOT EXISTS Miner ("
        "id INT AUTO_INCREMENT PRIMARY KEY, "
        "Username VARCHAR(255) UNIQUE NOT NULL, "
        "Password VARCHAR(255) NOT NULL, "
        "Address VARCHAR(255) NOT NULL,"
        "Status ENUM('online', 'offline') DEFAULT 'offline',"
        "LastSeen TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP;";

    if (mysql_query(db_, createTableSQL))
    {
        std::cerr << "Failed to create table: " << mysql_error(db_) << std::endl;
        return false;
    }
    return true;
}

bool MinerManager::registerMiner(const std::string &username, const std::string &password, const std::string &address)
{
    if (!db_)
    {
        std::cerr << "Database connection is not established." << std::endl;
        return false;
    }
    std::string query = "INSERT INTO Miner (Username, Password, Address) VALUES ('" +
                        username + "', '" + password + "', '" + address + "');";

    std::lock_guard<std::mutex> lock(mutex_);
    if (miners_.find(username) != miners_.end())
    {
        std::cerr << "Miner " << username << " is already registered." << std::endl;
        return false;
    }
    if (!mysql_query(db_, query.c_str()))
    {
        // Success if mysql_query returns 0
        std::cerr << "Failed to insert miner: " << mysql_error(db_) << std::endl;
        return false;
    }
    std::cout << "Miner " << username << " registered successfully." << std::endl;
    return true;
}

bool MinerManager::connectMiner(const std::string &username, const std::string &password)
{
    if (!db_)
    {
        std::cerr << "Database connection is not established." << std::endl;
        return false;
    }

    std::string query = "SELECT Username, Password, Address FROM Miner WHERE Username = '" + username + "' ;";
    if (mysql_query(db_, query.c_str()))
    {
        std::cerr << "Failed to execute query: " << mysql_error(db_) << std::endl;
        return false;
    }
    MYSQL_RES *result = mysql_store_result(db_);
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr)
    {
        std::cerr << "User not regitered!" << std::endl;
        return false;
    }
    std::string loadedUsername = row[0];
    std::string loadedPassword = row[1];
    std::string loadedAddress = row[2];

    Miner miner(loadedUsername, loadedPassword, loadedAddress);
    if (!miner.verifyPwd(password))
    {
        std::cerr << "Wrong Password!" << std::endl;
        return false;
    }
    miners_.emplace(loadedAddress, Miner(loadedUsername, loadedPassword, loadedAddress));
    return true;
}

TCPServer::TCPServer(int port)
    : port_(port), serverSocket_(-1), isRunning_(false) {}

TCPServer::~TCPServer()
{
    stop();
}

bool TCPServer::start()
{
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0)
    {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Bind failed." << std::endl;
        return false;
    }

    if (listen(serverSocket_, 5) < 0)
    {
        std::cerr << "Listen failed." << std::endl;
        return false;
    }

    isRunning_ = true;
    std::cout << "Server started on port " << port_ << std::endl;

    while (isRunning_)
    {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket_, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0)
        {
            if (isRunning_)
            {
                std::cerr << "Accept failed." << std::endl;
            }
            continue;
        }

        std::cout << "New client connected." << std::endl;
        clientThreads_.emplace_back(&TCPServer::handleClient, this, clientSocket);
    }
    return true;
}

void TCPServer::stop()
{
    isRunning_ = false;
    if (serverSocket_ >= 0)
    {
        close(serverSocket_);
        serverSocket_ = -1;
    }

    for (auto &thread : clientThreads_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    std::cout << "Server stopped." << std::endl;
}

void TCPServer::handleClient(int clientSocket)
{
    std::string message;
    while (isRunning_)
    {
        message = receiveMessage(clientSocket);
        if (message.empty())
        {
            std::cout << "Client disconnected." << std::endl;
            break;
        }
        std::cout << "Received message: " << message << std::endl;
        sendMessage(clientSocket, "Message received: " + message);
    }
    close(clientSocket);
}

void TCPServer::sendMessage(int clientSocket, const std::string &message)
{
    send(clientSocket, message.c_str(), message.size(), 0);
}

std::string TCPServer::receiveMessage(int clientSocket)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0)
    {
        return "";
    }
    return std::string(buffer, bytesRead);
}
