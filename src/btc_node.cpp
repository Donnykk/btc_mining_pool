#include "btc_node.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

BTC_Node::BTC_Node() {}

BTC_Node::~BTC_Node() {}

std::string BTC_Node::getBestBlockHash() const
{
    return BestBlockHash;
}

int BTC_Node::getBlockHeight() const
{
    return BlockHeight;
}

double BTC_Node::getDifficulty() const
{
    return Difficulty;
}

void BTC_Node::setBestBlockHash(const std::string &hash)
{
    BestBlockHash = hash;
}

void BTC_Node::setBlockHeight(int height)
{
    BlockHeight = height;
}

void BTC_Node::setDifficulty(double diff)
{
    Difficulty = diff;
}

size_t BTC_Node::WriteCallback(void *contents, size_t size, size_t nmemb, std::string *response)
{
    size_t totalSize = size * nmemb;
    response->append((char *)contents, totalSize);
    return totalSize;
}

std::string BTC_Node::createGetBlockPayload()
{
    Json::Value requestJson;
    Json::Value params(Json::arrayValue);
    requestJson["jsonrpc"] = "2.0";
    requestJson["method"] = "getblockchaininfo";
    requestJson["id"] = "getblock.io";
    requestJson["params"] = params;
    Json::StreamWriterBuilder writer;
    return Json::writeString(writer, requestJson);
}

std::string BTC_Node::sendJsonRpcRequest(const std::string &url, const std::string &payload)
{
    CURL *curl;
    CURLcode res;
    std::string response;

    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, BTC_Node::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    return response;
}

void BTC_Node::parseAndPrintBlockInfo(const std::string &response)
{
    Json::Value responseJson;
    Json::CharReaderBuilder reader;
    std::string errs;
    std::istringstream stream(response);
    if (Json::parseFromStream(reader, stream, &responseJson, &errs))
    {
        std::cout << "Block Information:" << std::endl;
        std::cout << responseJson.toStyledString() << std::endl;
        // Extract the "result" field
        const Json::Value result = responseJson["result"];
        if (!result.isNull())
        {
            // Extract and store values in the class's member variables
            BestBlockHash = result["bestblockhash"].asString();
            BlockHeight = result["blocks"].asInt();
            Difficulty = result["difficulty"].asDouble();

            // Print extracted information
            std::cout << "Parsed Information:" << std::endl;
            std::cout << "Best Block Hash: " << BestBlockHash << std::endl;
            std::cout << "Block Height: " << BlockHeight << std::endl;
            std::cout << "Difficulty: " << Difficulty << std::endl;
        }
        else
        {
            std::cerr << "JSON response does not contain 'result' field." << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to parse JSON response: " << errs << std::endl;
    }
}

void BTC_Node::startPoll(int intervalSeconds)
{
    while (true)
    {
        std::string rpcUrl = "https://go.getblock.io/fac1ed1394ed48e7b09e81bf7ddd4f86";

        std::cout << "Polling blockchain info every " << intervalSeconds << " seconds..." << std::endl;

        // Create JSON-RPC payload
        std::string payload = createGetBlockPayload();

        // Send request and parse response
        std::string response = sendJsonRpcRequest(rpcUrl, payload);

        parseAndPrintBlockInfo(response);

        // Wait for the next polling interval
        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
    }
}