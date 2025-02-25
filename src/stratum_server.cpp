// stratum_server.cpp
#include "stratum_server.h"
#include <iostream>
#include <sstream>
#include <mutex>
#include <unistd.h>
#include <unordered_set>

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
        handleMiningSubscribe(clientSocket, root["id"]);
    }
    else if (method == "mining.authorize")
    {
        handleMiningAuthorize(clientSocket, root["id"], root["params"]);
    }
    else if (method == "mining.notify")
    {
        handleMiningNotify(clientSocket);
    }
    else if (method == "mining.submit")
    {
        handleMiningSubmit(clientSocket, root["id"], root["params"]);
    }
    else
    {
        sendMessage(clientSocket, R"({"id": null, "error": "Unknown method."})");
    }
}

void StratumServer::handleMiningSubscribe(int clientSocket, const Json::Value &reqId)
{
    std::cout << "Worker subscribed." << std::endl;
    std::string sdiffSession = "b4b6693b72a50c7116db18d6497cac52";
    std::string notifySession = "ae6812eb4cd7735a302a8a9dd95cf71f";
    std::string extranonce1 = "08000002"; // extranonce1
    int extranonce2Size = 4;              // extranonce2 大小

    std::ostringstream oss;
    oss << R"({"id":)" << reqId.asInt();
    oss << R"(,"result":[[)";
    oss << "[\"mining.set_difficulty\",\"" << sdiffSession << "\"],";
    oss << "[\"mining.notify\",\"" << notifySession << "\"]],";
    oss << "\"" << extranonce1 << "\"," << extranonce2Size;
    oss << R"(],"error":null})";
    std::string response = oss.str();
    response += "\n";
    sendMessage(clientSocket, response);
    std::cout << "Send message: " << response << std::endl;
}

void StratumServer::handleMiningAuthorize(int clientSocket, const Json::Value &reqId, const Json::Value &params)
{
    std::string username = params[0].asString();
    std::string password = params[1].asString();

    std::cout << "Authorizing worker " << username << std::endl;

    std::ostringstream oss;
    oss << R"({"id":)" << reqId.asInt() << R"(,"result":true,"error":null})";
    std::string response = oss.str() + "\n";
    sendMessage(clientSocket, response);
    std::cout << "Send message: " << response << std::endl;
}

void StratumServer::handleMiningNotify(int clientSocket)
{
    // std::string notifyMessage = R"({
    //     "id": null,
    //     "method": "mining.notify",
    //     "params": [
    //         "job_id",
    //         "prevhash",
    //         "coinb1",
    //         "coinb2",
    //         [],
    //         "version",
    //         "nbits",
    //         "ntime",
    //         true
    //     ]
    // })";

    std::string notifyMessage = R"({
        "id": null,
        "method": "mining.notify",
        "params": [
            "job_2009",                
            "000000000019d6689c085ae165831e93",  
            "04ffff001d0104",          
            "ffffffff",                
            [],                        
            "1",                       
            "1d00ffff",               
            "1231006505",              
            true                       
        ]
    })";
    notifyMessage += "\n";
    sendMessage(clientSocket, notifyMessage);
    std::cout << "Send message: " << notifyMessage << std::endl;
}

void StratumServer::handleMiningSubmit(int clientSocket, const Json::Value &reqId, const Json::Value &params)
{
    std::string workerName = params[0].asString();
    std::string jobId = params[1].asString();
    std::string extraNonce = params[2].asString();
    std::string ntime = params[3].asString();
    std::string nonce = params[4].asString();

    std::cout << "Worker " << workerName << " submitted result for job " << jobId << std::endl;

    std::ostringstream oss;
    oss << R"({"id":)" << reqId.asInt() << R"(,"result":true,"error":null})";
    std::string response = oss.str() + "\n";
    sendMessage(clientSocket, response);
}
