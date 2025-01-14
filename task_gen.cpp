#include "task_gen.h"
#include "block_gen.h"
#include <iostream>
#include <sstream>
#include <json/json.h>

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

TaskGenerator::TaskGenerator(const std::string &brokers, const std::string &topic)
    : kafkaServer_(brokers, topic)
{
    if (!kafkaServer_.setupProducer())
    {
        std::cerr << "Failed to setup Kafka producer." << std::endl;
        exit(EXIT_FAILURE);
    }
}

std::string TaskGenerator::generateTask(const std::string &previousHash, double difficulty)
{
    BlockHeader blockHeader;
    blockHeader.previousHash = previousHash;
    blockHeader.timestamp = static_cast<uint32_t>(time(nullptr));
    blockHeader.nonce = 0;
    blockHeader.difficultyTarget = calculateDifficultyTarget(difficulty);
    std::vector<std::string> transactions = getTransactions();
    blockHeader.merkleRoot = calculateMerkleRoot(transactions);

    Json::Value taskJson;
    taskJson["previousHash"] = blockHeader.previousHash;
    taskJson["merkleRoot"] = blockHeader.merkleRoot;
    taskJson["timestamp"] = blockHeader.timestamp;
    taskJson["nonce"] = blockHeader.nonce;
    taskJson["difficultyTarget"] = blockHeader.difficultyTarget;

    Json::StreamWriterBuilder writer;
    return Json::writeString(writer, taskJson);
}

void TaskGenerator::pushMiningTask(const std::string &task)
{
    kafkaServer_.sendMessage(task);
}