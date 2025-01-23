// stratum_server.cpp
#include "stratum_server.h"
#include <iostream>
#include <sstream>
#include <mutex>
#include <unistd.h>
#include <unordered_set>

StratumServer::StratumServer(int port) : TCPServer(port) {}

StratumServer::~StratumServer() {}

void StratumServer::broadcastToMiners(const std::string &task)
{
    std::lock_guard<std::mutex> lock(clientMutex_);
    for (int clientSocket : clientSockets_)
    {
        sendMessage(clientSocket, task);
    }
}

void StratumServer::handleClient(int clientSocket)
{
    while (isRunning_)
    {
        std::string message = receiveMessage(clientSocket);

        if (message.empty())
        {
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        std::cout << "Received Stratum message: " << message << std::endl;

        // Process the Stratum message
        processStratumMessage(clientSocket, message);
    }

    close(clientSocket);
    std::lock_guard<std::mutex> lock(clientMutex_);
    clientSockets_.erase(clientSocket);
}

void StratumServer::processStratumMessage(int clientSocket, const std::string &message)
{
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;
    std::istringstream stream(message);

    if (!Json::parseFromStream(reader, stream, &root, &errs))
    {
        std::cerr << "Invalid Stratum message: " << errs << std::endl;
        sendMessage(clientSocket, R"({"id": null, "error": "Invalid message format."})");
        return;
    }

    std::string method = root["method"].asString();

    if (method == "mining.subscribe")
    {
        handleMiningSubscribe(clientSocket);
    }
    else if (method == "mining.authorize")
    {
        handleMiningAuthorize(clientSocket, root["params"]);
    }
    else if (method == "mining.notify")
    {
        handleMiningNotify(clientSocket);
    }
    else if (method == "mining.submit")
    {
        handleMiningSubmit(clientSocket, root["params"]);
    }
    else
    {
        sendMessage(clientSocket, R"({"id": null, "error": "Unknown method."})");
    }
}

void StratumServer::handleMiningNotify(int clientSocket)
{
    std::string notifyMessage = R"({
        "id": null,
        "method": "mining.notify",
        "params": [
            "job_id",
            "prevhash",
            "coinb1",
            "coinb2",
            [],
            "version",
            "nbits",
            "ntime",
            true
        ]
    })";
    sendMessage(clientSocket, notifyMessage);
}

void StratumServer::handleMiningSubmit(int clientSocket, const Json::Value &params)
{
    std::string workerName = params[0].asString();
    std::string jobId = params[1].asString();
    std::string extraNonce = params[2].asString();
    std::string ntime = params[3].asString();
    std::string nonce = params[4].asString();

    std::cout << "Worker " << workerName << " submitted result for job " << jobId << std::endl;

    // Return success for now (validation logic to be implemented)
    sendMessage(clientSocket, R"({"id": 3, "result": true, "error": null})");
}

void StratumServer::handleMiningAuthorize(int clientSocket, const Json::Value &params)
{
    std::string username = params[0].asString();
    std::string password = params[1].asString();

    // Example: always authorize (extend this with real authentication)
    std::cout << "Authorizing worker " << username << std::endl;
    sendMessage(clientSocket, R"({"id": 2, "result": true, "error": null})");
}

void StratumServer::handleMiningSubscribe(int clientSocket)
{
    std::cout << "Worker subscribed." << std::endl;
    sendMessage(clientSocket, R"({"id": 1, "result": true, "error": null})");
}
