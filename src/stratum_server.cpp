// stratum_server.cpp
#include "stratum_server.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <unistd.h>
#include <unordered_set>
#include "task_validator.h"

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
    if (result != SQLITE_OK)
    {
        std::cerr << "Could not open database: " << sqlite3_errmsg(db_) << std::endl;
    }
    else
    {
        if (!initDatabase())
        {
            std::cerr << "Failed to initialize database." << std::endl;
        }
    }
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
    const char *updateSQL = "UPDATE Miner SET Status = 'online', LastSeen = CURRENT_TIMESTAMP WHERE Username = ?;";
    sqlite3_stmt *updateStmt;
    result = sqlite3_prepare_v2(db_, updateSQL, -1, &updateStmt, nullptr);
    if (result == SQLITE_OK)
    {
        sqlite3_bind_text(updateStmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(updateStmt);
        sqlite3_finalize(updateStmt);
    }
    else
    {
        std::cerr << "Failed to update miner status: " << sqlite3_errmsg(db_) << std::endl;
    }
    miners_.emplace(loadedAddress, miner);
    sqlite3_finalize(stmt);
    return true;
}

bool MinerManager::disconnectMiner(const std::string &username)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const char *updateSQL = "UPDATE Miner SET Status = 'offline', LastSeen = CURRENT_TIMESTAMP WHERE Username = ?;";
    sqlite3_stmt *stmt;
    int result = sqlite3_prepare_v2(db_, updateSQL, -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        std::cerr << "Failed to prepare disconnect statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE)
    {
        std::cerr << "Failed to mark miner offline: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);

    auto it = miners_.find(username);
    if (it != miners_.end())
    {
        miners_.erase(it);
    }

    std::cout << "Miner " << username << " marked as offline." << std::endl;
    return true;
}

StratumServer::StratumServer(int port) : TCPServer(port), minerManager("mining_pool.db") {}

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
            std::string username = "root";
            std::cout << "Client " << username << " disconnected." << std::endl;
            minerManager.disconnectMiner(username);
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
    else if (method == "mining.extranonce.subscribe")
    {
        handleMiningExtranonceSubscribe(clientSocket, root["id"]);
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

    bool success = minerManager.connectMiner(username, password);

    std::ostringstream oss;
    if (success)
    {
        oss << R"({"id":)" << reqId.asInt() << R"(,"result":true,"error":null})";
    }
    else
    {
        oss << R"({"id":)" << reqId.asInt() << R"(,"result":false,"error":"Authentication failed"})";
    }
    std::string response = oss.str() + "\n";
    sendMessage(clientSocket, response);
    std::cout << "Send message: " << response << std::endl;
}

void StratumServer::handleMiningExtranonceSubscribe(int clientSocket, const Json::Value &reqId)
{
    std::ostringstream oss;
    oss << R"({"id":)" << reqId.asInt() << R"(,"result":true,"error":null})";
    std::string response = oss.str() + "\n";
    std::cout << "Sending message: " << response << std::endl;
    sendMessage(clientSocket, response);
    std::cout << "Send message: " << response << std::endl;
    handleMiningNotify(clientSocket);
}

std::string targetToNBits(const std::string &target)
{
    // 去除前导零
    size_t firstNonZero = target.find_first_not_of('0');
    if (firstNonZero == std::string::npos)
    {
        return "1d00ffff";
    }

    // 计算有效位数
    std::string effectiveTarget = target.substr(firstNonZero);

    // 计算指数（第一个字节）
    int exponent = (target.length() - firstNonZero) / 2;

    // 取前6个有效字符作为系数（3个字节）
    std::string coefficient = effectiveTarget.substr(0, 6);
    while (coefficient.length() < 6)
    {
        coefficient += "0";
    }

    // 组合成 nBits 格式（4字节，大端序）
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << exponent;
    ss << coefficient;

    return ss.str();
}

void StratumServer::handleMiningNotify(int clientSocket)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT JobId, Coinbase, Merkle, PrevBlock, Target "
                      "FROM Job WHERE Status = 'active' "
                      "ORDER BY Timestamp DESC LIMIT 1;";

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "\033[31m[ERROR]\033[0m Failed to prepare statement: "
                  << sqlite3_errmsg(db_) << std::endl;
        return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string jobId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string coinbase = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string merkle = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        std::string prevBlock = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        std::string target = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

        // 计算 coinbase 分割点
        size_t scriptStart =
            8 +  // Version
            2 +  // Input count
            64 + // Previous transaction hash
            8 +  // Previous output index
            2;   // Script length

        // coinbase2 开始位置
        size_t scriptEnd = scriptStart +
                           8 +  // Sequence
                           2 +  // Output count
                           16 + // Amount
                           2 +  // Script length
                           50 + // Output script
                           8;   // Locktime

        // 构造 merkle 分支数组字符串
        std::string merkleArrayStr = "[";
        std::istringstream merkleStream(merkle);
        std::string hash;
        bool first = true;
        while (std::getline(merkleStream, hash, ','))
        {
            if (!hash.empty())
            {
                if (!first)
                    merkleArrayStr += ",";
                merkleArrayStr += "\"" + hash + "\"";
                first = false;
            }
        }
        merkleArrayStr += "]";

        // 将 target 转换为 nBits 格式
        std::string nBits = targetToNBits(target);

        // 使用原始字符串构造完整的消息
        std::ostringstream oss;
        oss << R"({"id":null,"method":"mining.notify","params":[")"
            << jobId << "\",\""
            << prevBlock << "\",\""
            << coinbase.substr(0, scriptStart) << "\",\"" // coinbase1
            << coinbase.substr(scriptEnd) << "\","        // coinbase2
            << merkleArrayStr << ",\""
            << "20000000" << "\",\""
            << nBits << "\",\""
            << std::hex << time(nullptr) << "\","
            << "true]}";

        std::string message = oss.str() + "\n";

        std::cout << "\033[32m[>]\033[0m Sending notify: " << message;
        sendMessage(clientSocket, message);
    }
    else
    {
        std::cerr << "\033[31m[Error]\033[0m No active mining tasks found." << std::endl;
    }

    sqlite3_finalize(stmt);
}

void StratumServer::handleMiningSubmit(int clientSocket, const Json::Value &reqId, const Json::Value &params)
{
    std::string workerName = params[0].asString();
    std::string jobId = params[1].asString();
    std::string extraNonce = params[2].asString();
    std::string ntime = params[3].asString();
    std::string nonce = params[4].asString();

    std::cout << "Worker " << workerName << " submitted result for job " << jobId << std::endl;
    TaskValidator validator;
    bool valid = validator.validate(workerName, jobId, extraNonce, ntime, nonce);

    std::ostringstream oss;
    oss << R"({"id":)" << reqId.asInt() << R"(,"result":true,"error":null})";
    std::string response = oss.str() + "\n";
    sendMessage(clientSocket, response);
}

void StratumServer::wait()
{
    while (isRunning_)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void StratumServer::stop()
{
    isRunning_ = false;
    TCPServer::stop();
}

StratumServer *g_server = nullptr;

void signalHandler(int signal)
{
    std::cout << "\033[33m[信号]\033[0m 收到信号: " << signal << ", 正在停止服务..." << std::endl;
    if (g_server)
    {
        g_server->stop();
    }
}

int main()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try
    {
        int stratumPort = 3333;

        std::cout << "\033[32m[启动]\033[0m Stratum 服务启动" << std::endl;
        std::cout << "├── 监听端口: " << stratumPort << std::endl;
        std::cout << "├── 数据库: mining_pool.db" << std::endl;
        std::cout << "└── 等待矿工连接..." << std::endl;

        // 初始化 Stratum 服务器
        StratumServer stratumServer(stratumPort);
        g_server = &stratumServer;

        // 启动服务
        if (!stratumServer.start())
        {
            std::cerr << "\033[31m[错误]\033[0m Stratum 服务启动失败" << std::endl;
            return 1;
        }

        // 主线程等待服务停止
        stratumServer.wait();

        std::cout << "\033[32m[停止]\033[0m Stratum 服务已停止" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\033[31m[错误]\033[0m Stratum 服务初始化失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}