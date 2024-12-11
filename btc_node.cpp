#include "btc_node.h"
#include <iostream>
#include <sstream>

size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *response)
{
    size_t totalSize = size * nmemb;
    response->append((char *)contents, totalSize);
    return totalSize;
}

std::string sendJsonRpcRequest(const std::string &url, const std::string &payload)
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
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
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

std::string createGetBlockPayload()
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

void parseAndPrintBlockInfo(const std::string &response)
{
    Json::Value responseJson;
    Json::CharReaderBuilder reader;
    std::string errs;
    std::istringstream stream(response);
    if (Json::parseFromStream(reader, stream, &responseJson, &errs))
    {
        std::cout << "Block Information:" << std::endl;
        std::cout << responseJson.toStyledString() << std::endl;
    }
    else
    {
        std::cerr << "Failed to parse JSON response: " << errs << std::endl;
    }
}