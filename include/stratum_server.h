// stratum_server.h
#ifndef STRATUM_SERVER_H
#define STRATUM_SERVER_H

#include "tcp_server.h"
#include <string>
#include <json/json.h>
#include <unordered_set>

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
    MinerManager(const std::string &dbPath);
    ~MinerManager();
    bool registerMiner(const std::string &username, const std::string &password, const std::string &address);
    bool connectMiner(const std::string &username, const std::string &password);
    bool disconnectMiner(const std::string &username);

private:
    std::unordered_map<std::string, Miner> miners_;
    std::mutex mutex_;

    sqlite3 *db_;
    bool initDatabase();
};

class StratumServer : public TCPServer
{
public:
    StratumServer(int port);
    ~StratumServer();

    // Broadcast task to all connected miners
    void broadcastToMiners(const std::string &task);

protected:
    void handleClient(int clientSocket) override;

private:
    MinerManager minerManager;

    std::mutex clientMutex_;
    std::unordered_set<int> clientSockets_;

    // Process a received Stratum message
    void processStratumMessage(int clientSocket, const std::string &message);

    // Handle specific Stratum methods
    void handleMiningSubscribe(int clientSocket, const Json::Value &reqId);
    void handleMiningAuthorize(int clientSocket, const Json::Value &reqId, const Json::Value &params);
    void handleMiningExtranonceSubscribe(int clientSocket, const Json::Value &reqId);
    void handleMiningNotify(int clientSocket);
    void handleMiningSubmit(int clientSocket, const Json::Value &reqId, const Json::Value &params);
};

#endif // STRATUM_SERVER_H
