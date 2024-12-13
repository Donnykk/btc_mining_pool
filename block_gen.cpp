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

std::string toHexString(uint64_t number, int totalBits)
{
    std::ostringstream oss;
    oss << std::hex << number;
    std::string hexString = oss.str();
    while (hexString.length() < totalBits / 4)
    {
        hexString = "0" + hexString;
    }
    return hexString;
}

// Calculate Difficulty Target
int calculateDifficultyTarget(double difficulty)
{
    // Maximum target (difficulty = 1)
    const uint64_t maxTargetHigh = 0xFFFF;
    const int shiftBits = 208;
    double currentTarget = (maxTargetHigh * std::pow(2, shiftBits)) / difficulty;
    // Convert current target to binary string
    std::string hexTarget = toHexString(static_cast<uint64_t>(currentTarget), 256);
    int leadingZeros = 0;
    for (char c : hexTarget)
    {
        if (c == '0')
            leadingZeros++;
        else
            break;
    }
    return leadingZeros / 10;
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

// BlockGenerator method implementation
BlockHeader BlockGenerator::generateBlock()
{
    // Generate mining task using TaskGenerator
    TaskGenerator taskGenerator;
    BlockHeader blockHeader = taskGenerator.generateTask(previousHash, difficulty);

    // TODO：Allocate to miners
    std::string targetPrefix(blockHeader.difficultyTarget, '0');
    std::cout << "DifficultyTarget: " << blockHeader.difficultyTarget << std::endl;
    while (true)
    {
        std::string blockHash = sha256(blockHeader.toString());
        if (blockHash.substr(0, blockHeader.difficultyTarget) == targetPrefix)
        {
            std::cout << "Found valid nonce: " << blockHeader.nonce << "\n";
            std::cout << "Block Hash: " << blockHash << "\n";
            break;
        }
        blockHeader.nonce++;
    }
    return blockHeader;
}

BlockHeader TaskGenerator::generateTask(const std::string &previousHash, double difficulty)
{
    BlockHeader blockHeader;
    blockHeader.previousHash = previousHash;
    blockHeader.timestamp = static_cast<uint32_t>(time(nullptr));
    blockHeader.nonce = 0;
    blockHeader.difficultyTarget = calculateDifficultyTarget(difficulty);
    std::vector<std::string> transactions = getTransactions();
    blockHeader.merkleRoot = calculateMerkleRoot(transactions);
    return blockHeader;
}
