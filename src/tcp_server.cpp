#include "tcp_server.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

void HttpResponse::setStatus(int code)
{
    status = code;
}

void HttpResponse::setContent(const std::string &data)
{
    content = data;
}

TCPServer::TCPServer(int port)
    : port_(port), serverSocket_(-1), isRunning_(false)
{
    if (sqlite3_open("mining_pool.db", &db_) != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        return;
    }
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
    std::string message = receiveMessage(clientSocket);
    if (!message.empty())
    {
        std::cout << "Received request:\n"
                  << message << std::endl;

        HttpRequest request = parseHttpRequest(message);
        std::cout << "Method: " << request.method << ", Path: " << request.path << std::endl;

        HttpResponse response;
        handleHttpRequest(clientSocket, request);
    }

    close(clientSocket);
    std::cout << "Client disconnected." << std::endl;
}

HttpRequest TCPServer::parseHttpRequest(const std::string &message)
{
    HttpRequest request;
    std::istringstream iss(message);
    std::string line;

    // 解析请求行
    if (std::getline(iss, line))
    {
        std::istringstream lineStream(line);
        lineStream >> request.method >> request.path;
    }

    // 解析请求头
    while (std::getline(iss, line) && line != "\r" && line != "")
    {
        size_t pos = line.find(':');
        if (pos != std::string::npos)
        {
            std::string name = line.substr(0, pos);
            std::string value = line.substr(pos + 2);
            request.headers[name] = value;
        }
    }

    return request;
}

void TCPServer::handleHttpRequest(int clientSocket, const HttpRequest &request)
{
    HttpResponse response;

    // 处理 OPTIONS 预检请求
    if (request.method == "OPTIONS")
    {
        response.setStatus(200);
        response.headers["Access-Control-Allow-Origin"] = "*";
        response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
        response.headers["Access-Control-Allow-Headers"] = "Content-Type, Accept";
        response.headers["Access-Control-Max-Age"] = "86400";
        response.headers["Content-Type"] = "text/plain";
        response.setContent("");

        std::string responseStr = formatHttpResponse(response);
        sendMessage(clientSocket, responseStr);
        return;
    }

    // 处理正常请求
    std::string path = request.path;
    if (request.method == "GET")
    {
        if (path.find("/api/miner/status") == 0)
        {
            handleMinersRequest(request, response);
        }
        else
        {
            response.setStatus(404);
            response.setContent("{\"error\":\"Not Found\"}");
        }
    }
    else
    {
        response.setStatus(405);
        response.setContent("{\"error\":\"Method Not Allowed\"}");
    }

    // 发送响应
    std::string responseStr = formatHttpResponse(response);
    sendMessage(clientSocket, responseStr);
}

std::string TCPServer::formatHttpResponse(const HttpResponse &response)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 " << response.status << " ";

    if (response.status == 200)
        oss << "OK";
    else if (response.status == 404)
        oss << "Not Found";
    else if (response.status == 500)
        oss << "Internal Server Error";
    else if (response.status == 400)
        oss << "Bad Request";

    oss << "\r\n";

    // 确保每个响应都有这些 CORS 头
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type, Accept\r\n";
    oss << "Vary: Origin\r\n"; // 添加 Vary 头

    // 添加其他响应头
    for (const auto &header : response.headers)
    {
        oss << header.first << ": " << header.second << "\r\n";
    }

    oss << "Connection: keep-alive\r\n"; // 添加 Connection 头
    oss << "\r\n"
        << response.content;

    return oss.str();
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

void TCPServer::handleMinersRequest(const HttpRequest &request, HttpResponse &response)
{
    // 从 URL 中解析用户名参数
    std::string username;
    size_t pos = request.path.find('?');
    if (pos != std::string::npos)
    {
        std::string params = request.path.substr(pos + 1);
        std::string prefix = "username=";
        if (params.find(prefix) == 0)
        {
            username = params.substr(prefix.length());
        }
    }

    std::cout << "Username: " << username << std::endl;
    if (username.empty())
    {
        response.setStatus(400);
        response.setContent("{\"error\":\"Missing miner username\"}");
        return;
    }

    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT Username, Address, Status, ValidShares, HashRate, TotalReward "
                      "FROM Miner WHERE Username = ?;";

    Json::Value result(Json::objectValue);

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        response.setStatus(500);
        response.setContent("{\"error\":\"Database error\"}");
        return;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result["username"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        result["address"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        result["status"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        result["validshares"] = sqlite3_column_int(stmt, 3);
        result["hashrate"] = sqlite3_column_double(stmt, 4);
        result["totalreward"] = sqlite3_column_double(stmt, 5);

        response.setStatus(200);
        response.headers["Content-Type"] = "application/json";
        response.setContent(Json::FastWriter().write(result));
        std::cout << "Miner status: " << result.toStyledString() << std::endl;
    }
    else
    {
        response.setStatus(404);
        response.setContent("{\"error\":\"Miner not found\"}");
    }

    sqlite3_finalize(stmt);
}
