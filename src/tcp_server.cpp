#include "tcp_server.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

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
