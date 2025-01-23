#ifndef BTC_NODE_H
#define BTC_NODE_H

#include <string>
#include <curl/curl.h>
#include <json/json.h>

class BTC_Node
{
private:
    std::string BestBlockHash;
    int BlockHeight;
    double Difficulty;

public:
    BTC_Node();
    ~BTC_Node();

    // Getter methods
    std::string getBestBlockHash() const;
    int getBlockHeight() const;
    double getDifficulty() const;

    // Setter methods
    void setBestBlockHash(const std::string &hash);
    void setBlockHeight(int height);
    void setDifficulty(double diff);

    // Write the response data into a string
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *response);

    // Create JSON-RPC payload
    std::string createGetBlockPayload();

    // Send JSON-RPC request
    std::string sendJsonRpcRequest(const std::string &url, const std::string &payload);

    // Parse and print block information
    void parseAndPrintBlockInfo(const std::string &response);

    // Poll to get new block
    void startPoll(int intervalSeconds);
};

#endif // BTC_NODE_H
