#include "btc_node.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <sqlite3.h>
#include "ConfigManager.h"

BTC_Node::BTC_Node()
{
    int result = sqlite3_open("mining_pool.db", &db_);
    if (result != SQLITE_OK)
    {
        std::cerr << "[ERROR] Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
    }
    else
    {
        std::cout << "[INFO] Database opened successfully!" << std::endl;
    }

    // 从配置文件获取 Kafka 配置
    auto &config = ConfigManager::getInstance();
    std::string kafkaBrokers = "localhost:9092";
    std::string kafkaTopic = "BTC_blocks";

    if (!initKafka(kafkaBrokers, kafkaTopic))
    {
        std::cerr << "[ERROR] Failed to initialize Kafka" << std::endl;
    }
}

BTC_Node::~BTC_Node()
{
    if (db_)
    {
        sqlite3_close(db_);
        std::cout << "[INFO] Database closed successfully!" << std::endl;
    }
}

// 回调函数，把服务器返回的数据存到 string 里
size_t WriteCallback(void *ptr, size_t size, size_t nmemb, std::string *data)
{
    if (!ptr || !data)
        return 0;
    data->append((char *)ptr, size * nmemb);
    return size * nmemb;
}

std::string BTC_Node::sendJsonRpcRequest(const std::string &method, const std::vector<std::string> &params)
{
    try
    {
        Json::Value requestJson;
        requestJson["jsonrpc"] = "2.0";
        requestJson["id"] = "btc-node";
        requestJson["method"] = method;
        requestJson["params"] = Json::Value(Json::arrayValue);
        for (const auto &param : params)
        {
            if (std::all_of(param.begin(), param.end(), ::isdigit))
            {
                requestJson["params"].append(std::stoi(param));
            }
            else
            {
                requestJson["params"].append(param);
            }
        }

        Json::StreamWriterBuilder writer;
        std::string payload = Json::writeString(writer, requestJson);

        CURL *curl = curl_easy_init();
        if (!curl)
        {
            throw std::runtime_error("Failed to initialize CURL");
        }

        std::string response;
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        std::string api_key = ConfigManager::getInstance().getValue("GETBLOCK_API_KEY");
        if (api_key.empty())
        {
            throw std::runtime_error("GETBLOCK_API_KEY not found");
        }

        std::string url = "https://go.getblock.io/" + api_key;
        //std::cout << "[DEBUG] Using URL: " << url << std::endl;

        // CURL 设置
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        //std::cout << "[DEBUG] Sending RPC request: " << payload << std::endl;

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            std::string error = curl_easy_strerror(res);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            throw std::runtime_error("CURL request failed: " + error);
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (http_code != 200)
        {
            throw std::runtime_error("HTTP request failed with code: " + std::to_string(http_code));
        }

        return response;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Exception in sendJsonRpcRequest: " << e.what() << std::endl;
        return "";
    }
}

void BTC_Node::parseBlockInfo(const std::string &response)
{
    Json::Value jsonData;
    Json::CharReaderBuilder reader;
    std::string errs;
    std::istringstream stream(response);
    if (!Json::parseFromStream(reader, stream, &jsonData, &errs))
    {
        std::cerr << "[ERROR] Failed to parse JSON response" << std::endl;
        return;
    }

    Json::Value result = jsonData["result"];
    if (result.isNull())
    {
        std::cerr << "[ERROR] Invalid JSON-RPC response" << std::endl;
        return;
    }

    bestBlockHash = result["hash"].asString();
    blockHeight = result["height"].asInt();
    difficulty = result["difficulty"].asDouble();
    target = result["target"].asString();
    timestamp = result["time"].asUInt();

    // 简化区块信息输出
    std::cout << "[INFO] New Block - Height: " << blockHeight << ", Hash: " << bestBlockHash << std::endl;

    // 静默处理交易列表
    transactions.clear();
    for (const auto &tx : result["tx"])
    {
        transactions.push_back(tx["txid"].asString());
    }

    // 构建区块信息的 JSON 消息
    Json::Value blockInfo;
    blockInfo["type"] = "new_block";
    blockInfo["height"] = blockHeight;
    blockInfo["hash"] = bestBlockHash;
    blockInfo["difficulty"] = difficulty;
    blockInfo["target"] = target;
    blockInfo["timestamp"] = timestamp;
    blockInfo["transactions"] = Json::Value(Json::arrayValue);
    for (const auto &tx : transactions)
    {
        blockInfo["transactions"].append(tx);
    }

    // 将 JSON 转换为字符串
    Json::StreamWriterBuilder writer;
    std::string message = Json::writeString(writer, blockInfo);

    // 通过 Kafka 发送消息
    if (kafka_ && kafka_->checkConnection())
    {
        kafka_->sendMessage(message);
        std::cout << "[INFO] Block info sent to Kafka" << std::endl;
    }
    else
    {
        std::cerr << "[ERROR] Failed to send block info to Kafka" << std::endl;
    }

    // 继续存储到数据库
    storeBlockData();
}

bool BTC_Node::storeBlockData()
{
    std::ostringstream txs;
    for (const auto &tx : transactions)
    {
        txs << tx << ",";
    }

    const char *sql = "INSERT OR REPLACE INTO Blocks (BlockHeight, BestBlockHash, Difficulty, Target, Timestamp, Transactions) "
                      "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "[ERROR] Failed to prepare SQL statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, blockHeight);
    sqlite3_bind_text(stmt, 2, bestBlockHash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, difficulty);
    sqlite3_bind_text(stmt, 4, target.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, timestamp);
    sqlite3_bind_text(stmt, 6, txs.str().c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "[ERROR] Failed to insert block data: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    std::cout << "[INFO] Stored block data for height " << blockHeight << std::endl;
    return true;
}

void BTC_Node::startPoll(int intervalSeconds)
{
    while (true)
    {
        std::string latestBlockHashResponse = sendJsonRpcRequest("getbestblockhash", {});
        if (latestBlockHashResponse.empty())
        {
            std::cerr << "[ERROR] Failed to retrieve latest block hash" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
            continue;
        }

        Json::Value jsonData;
        Json::CharReaderBuilder reader;
        std::string errs;
        std::istringstream stream(latestBlockHashResponse);
        if (!Json::parseFromStream(reader, stream, &jsonData, &errs))
        {
            std::cerr << "[ERROR] Failed to parse JSON response: " << errs << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
            continue;
        }

        std::string latestBlockHash = jsonData["result"].asString();
        if (latestBlockHash.empty())
        {
            std::cerr << "[ERROR] JSON-RPC response missing 'result' field!" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
            continue;
        }
        std::cout << "[DEBUG] Latest block hash: " << latestBlockHash << std::endl;

        // request block data
        std::string blockData = sendJsonRpcRequest("getblock", {latestBlockHash, "2"});
        if (!blockData.empty())
        {
            parseBlockInfo(blockData);
        }
        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
    }
}

bool BTC_Node::initKafka(const std::string &brokers, const std::string &topic)
{
    try
    {
        kafka_ = std::make_unique<KafkaServer>(brokers, topic);
        if (!kafka_->setupProducer())
        {
            std::cerr << "[ERROR] Failed to setup Kafka producer" << std::endl;
            return false;
        }
        std::cout << "[INFO] Kafka producer initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Failed to initialize Kafka: " << e.what() << std::endl;
        return false;
    }
}