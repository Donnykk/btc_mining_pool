#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <netinet/in.h>
#include <mysql/mysql.h>

class Miner
{
public:
    Miner(const std::string &username, const std::string &password, const std::string &address);

    std::string getUsername() const;
    std::string getAddress() const;

    bool verifyPwd(const std::string &password) const;

private:
    std::string username_;
    std::string password_;
    std::string address_;
};

class MinerManager
{
public:
    MinerManager(const std::string &dbHost, const std::string &dbUser, const std::string &dbPassword, const std::string &dbName);

    bool registerMiner(const std::string &username, const std::string &password, const std::string &address);
    bool connectMiner(const std::string &username, const std::string &password);

private:
    std::unordered_map<std::string, Miner> miners_;
    std::mutex mutex_;

    MYSQL *db_;
    bool initDatabase();
};

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

    virtual void handleClient(int clientSocket);

    void sendMessage(int clientSocket, const std::string &message);
    std::string receiveMessage(int clientSocket);
};

#endif // TCPSERVER_H