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

MinerManager::MinerManager(const std::string &dbPath)
{
    int result = sqlite3_open(dbPath.c_str(), &db_);
}

MinerManager::~MinerManager()
{
    if (db_)
    {
        sqlite3_close(db_);
    }
}

bool MinerManager::initDatabase()
{
    const char *createTableSQL =
        "CREATE TABLE IF NOT EXISTS Miner ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "Username TEXT UNIQUE NOT NULL, "
        "Password TEXT NOT NULL, "
        "Address TEXT NOT NULL, "
        "Status TEXT DEFAULT 'offline', "
        "LastSeen TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";

    char *errMsg = nullptr;
    int result = sqlite3_exec(db_, createTableSQL, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK)
    {
        std::cerr << "Failed to create table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool MinerManager::registerMiner(const std::string &username, const std::string &password, const std::string &address)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (miners_.find(username) != miners_.end())
    {
        std::cerr << "Miner " << username << " is already registered." << std::endl;
        return false;
    }

    const char *insertSQL = "INSERT INTO Miner (Username, Password, Address) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    int result = sqlite3_prepare_v2(db_, insertSQL, -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, address.c_str(), -1, SQLITE_STATIC);

    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE)
    {
        std::cerr << "Failed to insert miner: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    std::cout << "Miner " << username << " registered successfully." << std::endl;
    return true;
}

bool MinerManager::connectMiner(const std::string &username, const std::string &password)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char *selectSQL = "SELECT Username, Password, Address FROM Miner WHERE Username = ?;";
    sqlite3_stmt *stmt;
    int result = sqlite3_prepare_v2(db_, selectSQL, -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        std::cerr << "User not registered!" << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    std::string loadedUsername = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    std::string loadedPassword = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    std::string loadedAddress = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

    Miner miner(loadedUsername, loadedPassword, loadedAddress);
    if (!miner.verifyPwd(password))
    {
        std::cerr << "Wrong Password!" << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    miners_.emplace(loadedAddress, miner);
    sqlite3_finalize(stmt);
    return true;
}

TCPServer::TCPServer(int port)
    : port_(port), serverSocket_(-1), isRunning_(false)
{
    dbPath = "mining_pool.db";
}

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
