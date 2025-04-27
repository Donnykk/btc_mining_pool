#ifndef BTC_NODE_H
#define BTC_NODE_H

#include <string>
#include <vector>
#include <curl/curl.h>
#include <json/json.h>
#include <sqlite3.h>
#include "kafka_server.h"

class BTC_Node
{
private:
    std::string bestBlockHash;
    int blockHeight;
    double difficulty;
    std::string merkleRoot;
    std::string target;
    uint32_t timestamp;
    std::vector<std::string> transactions;
    std::string coinbaseTx;
    std::string prevBlockHash;
    sqlite3 *db_;
    std::unique_ptr<KafkaServer> kafka_;

public:
    BTC_Node();
    ~BTC_Node();

    bool running_ = true;

    // Send JSON-RPC request
    std::string sendJsonRpcRequest(const std::string &method, const std::vector<std::string> &params);

    // Parse block information
    void parseBlockInfo(const std::string &response);

    // Poll to get new blocks
    void startPoll(int intervalSeconds);

    void stop() { running_ = false; }

    // Store block data into database
    bool storeBlockData();

    // Initialize Kafka
    bool initKafka(const std::string &brokers, const std::string &topic);
};

#endif // BTC_NODE_H