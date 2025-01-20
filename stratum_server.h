// stratum_server.h
#ifndef STRATUM_SERVER_H
#define STRATUM_SERVER_H

#include "tcp_server.h"
#include <string>
#include <json/json.h>

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
    // Process a received Stratum message
    void processStratumMessage(int clientSocket, const std::string &message);

    // Handle specific Stratum methods
    void handleMiningNotify(int clientSocket);
    void handleMiningSubmit(int clientSocket, const Json::Value &params);
    void handleMiningAuthorize(int clientSocket, const Json::Value &params);
    void handleMiningSubscribe(int clientSocket);
};

#endif // STRATUM_SERVER_H
