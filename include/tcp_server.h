#pragma once

#include <string>
#include <vector>
#include <thread>
#include <functional>
#include <map>
#include <sqlite3.h>
#include <json/json.h>
#include <sstream>

// HTTP请求结构
struct HttpRequest
{
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> parameters;
    std::string body;
};

// HTTP响应结构
struct HttpResponse
{
    int status = 200;
    std::map<std::string, std::string> headers;
    std::string content;

    void setStatus(int code);
    void setContent(const std::string &data);
};

class TCPServer
{
public:
    TCPServer(int port);
    virtual ~TCPServer();

    virtual bool start();
    virtual void stop();

protected:
    virtual void handleClient(int clientSocket);
    virtual void sendMessage(int clientSocket, const std::string &message);
    virtual std::string receiveMessage(int clientSocket);

    // HTTP 请求处理
    virtual HttpRequest parseHttpRequest(const std::string &message);
    virtual std::string formatHttpResponse(const HttpResponse &response);
    virtual void handleHttpRequest(int clientSocket, const HttpRequest &request);

    // API 处理方法
    virtual void handleMinersRequest(const HttpRequest &request, HttpResponse &response);

    int port_;
    int serverSocket_;
    bool isRunning_;
    std::vector<std::thread> clientThreads_;

    // 数据库连接
    sqlite3 *db_;
};
