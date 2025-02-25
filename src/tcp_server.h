#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <netinet/in.h>
#include <mysql/mysql.h>
#include <sqlite3.h>

class TCPServer
{
public:
    TCPServer(int port);
    ~TCPServer();

    bool start();
    void stop();

protected:
    int serverSocket_;
    int port_;

    std::atomic<bool> isRunning_;
    std::vector<std::thread> clientThreads_;
    std::mutex clientMutex_;
    std::unordered_set<int> clientSockets_;

    // database configure
    std::string dbHost = "localhost";
    std::string dbUser = "root";
    std::string dbPassword = "root";
    std::string dbName = "Mining_Pool";
    std::string dbPath = "mining_pool.db";

    virtual void handleClient(int clientSocket);

    void sendMessage(int clientSocket, const std::string &message);
    std::string receiveMessage(int clientSocket);
};

#endif // TCPSERVER_H