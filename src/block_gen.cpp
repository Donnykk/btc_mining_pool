#include "block_gen.h"
#include <sstream>

std::vector<std::string> getTransactions()
{
    // TODO: Use JSON-RPC API to get Txns
    std::vector<std::string> transactions;
    transactions.push_back("tx1: Alice -> Bob (0.1 BTC)");
    transactions.push_back("tx2: Charlie -> Dave (0.05 BTC)");
    transactions.push_back("tx3: Eve -> Frank (0.2 BTC)");
    return transactions;
}

// Calculate SHA256
std::string sha256(const std::string &data)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)data.c_str(), data.size(), hash);
    char hexStr[SHA256_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        snprintf(hexStr + i * 2, sizeof(hexStr) - i * 2, "%02x", hash[i]);
    }
    return std::string(hexStr);
}

// Merkle Root
std::string calculateMerkleRoot(const std::vector<std::string> &transactions)
{
    if (transactions.empty())
    {
        return "";
    }

    std::vector<std::string> currentLevel = transactions;
    while (currentLevel.size() > 1)
    {
        std::vector<std::string> nextLevel;
        for (size_t i = 0; i < currentLevel.size(); i += 2)
        {
            if (i + 1 < currentLevel.size())
            {
                nextLevel.push_back(sha256(currentLevel[i] + currentLevel[i + 1]));
            }
            else
            {
                nextLevel.push_back(sha256(currentLevel[i]));
            }
        }
        currentLevel = nextLevel;
    }
    return currentLevel[0];
}

// Fetch latest block hash
std::string getBestBlockHash()
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "https://blockchain.info/q/latesthash");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t
                         {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb; });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

std::string BlockHeader::toString() const
{
    return previousHash + merkleRoot + std::to_string(timestamp) + std::to_string(nonce);
}

// BlockGenerator constructor implementation
BlockGenerator::BlockGenerator(const std::string &previousHash, const double difficulty) : previousHash(previousHash), difficulty(difficulty) {}
