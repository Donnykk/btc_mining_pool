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

    // Function to write the response data into a string
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *response);

    // Function to create JSON-RPC payload
    std::string createGetBlockPayload();

    // Function to send JSON-RPC request
    std::string sendJsonRpcRequest(const std::string &url, const std::string &payload);

    // Function to parse and print block information
    void parseAndPrintBlockInfo(const std::string &response);
};

#endif // BTC_NODE_H
