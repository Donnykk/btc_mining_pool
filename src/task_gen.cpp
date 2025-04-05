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
    const uint64_t maxTargetHigh = 0xFFFF;
    const int shiftBits = 208;
    double currentTarget = (maxTargetHigh * std::pow(2, shiftBits)) / difficulty;
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

bool TaskGenerator::initDatabase()
{
    const char *createTableSQL =
        "CREATE TABLE IF NOT EXISTS Job ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "JobId TEXT UNIQUE NOT NULL,"
        "Coinbase TEXT NOT NULL, "
        "Merkle TEXT NOT NULL, "
        "PrevBlock TEXT NOT NULL, "
        "Target TEXT NOT NULL, "
        "Status TEXT DEFAULT 'active',"
        "Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    char *errMsg = nullptr;
    int result = sqlite3_exec(db_, createTableSQL, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK)
    {
        std::cerr << "Failed to create Job table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

TaskGenerator::TaskGenerator(const std::string &brokers, const std::string &topic)
    : kafkaServer_(brokers, topic), isListening_(false)
{
    // Set up database
    std::string dbPath = "mining_pool.db";
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK)
    {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
    }
    else
    {
        if (!initDatabase())
        {
            std::cerr << "Failed to initialize database." << std::endl;
        }
    }
    // Set up Kafka producer
    if (!kafkaServer_.setupProducer())
    {
        std::cerr << "Failed to setup Kafka producer." << std::endl;
        exit(EXIT_FAILURE);
    }
}

std::string TaskGenerator::generateCoinbaseTransaction()
{
    std::ostringstream oss;
    oss << "coinbase_tx_" << time(nullptr);
    return oss.str();
}

std::string TaskGenerator::generateTask(const std::string &previousHash, double difficulty)
{
    BlockHeader blockHeader;
    blockHeader.previousHash = previousHash;
    blockHeader.timestamp = static_cast<uint32_t>(time(nullptr));
    blockHeader.nonce = 0;
    blockHeader.difficultyTarget = calculateDifficultyTarget(difficulty);

    std::vector<std::string> transactions = getTransactions();
    std::string coinbaseTx = generateCoinbaseTransaction();
    transactions.insert(transactions.begin(), coinbaseTx);
    blockHeader.merkleRoot = calculateMerkleRoot(transactions);

    // 生成一个业务层的 JobId
    std::ostringstream jobIdStream;
    jobIdStream << blockHeader.previousHash << blockHeader.timestamp << blockHeader.nonce;
    std::string jobId = toHexString(std::hash<std::string>{}(jobIdStream.str()), 64);

    Json::Value taskJson;
    taskJson["JobId"] = jobId;
    taskJson["previousHash"] = blockHeader.previousHash;
    taskJson["merkleRoot"] = blockHeader.merkleRoot;
    taskJson["timestamp"] = blockHeader.timestamp;
    taskJson["nonce"] = blockHeader.nonce;
    taskJson["difficultyTarget"] = blockHeader.difficultyTarget;
    taskJson["coinbase"] = coinbaseTx;

    Json::StreamWriterBuilder writer;
    std::string taskStr = Json::writeString(writer, taskJson);

    bool stored = storeTask(jobId, coinbaseTx, blockHeader.merkleRoot, blockHeader.previousHash,
                            toHexString(static_cast<uint64_t>(blockHeader.difficultyTarget), 256));
    if (!stored)
    {
        std::cerr << "Failed to store task with JobId: " << jobId << std::endl;
    }
    return taskStr;
}

bool TaskGenerator::storeTask(const std::string &jobId,
                              const std::string &coinbase,
                              const std::string &merkle,
                              const std::string &prevBlock,
                              const std::string &target)
{
    const char *insertSQL = "INSERT INTO Job (JobId, Coinbase, Merkle, PrevBlock, Target) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    int result = sqlite3_prepare_v2(db_, insertSQL, -1, &stmt, nullptr);
    if (result != SQLITE_OK)
    {
        std::cerr << "Failed to prepare insert statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, jobId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, coinbase.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, merkle.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, prevBlock.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, target.c_str(), -1, SQLITE_TRANSIENT);

    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE)
    {
        std::cerr << "Failed to insert task: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    std::cout << "Task with JobId " << jobId << " stored successfully." << std::endl;
    return true;
}

void TaskGenerator::pushMiningTask(const std::string &task)
{
    kafkaServer_.sendMessage(task);
}

bool TaskGenerator::startBlockListener(const std::string &blockTopic)
{
    if (isListening_)
    {
        return true;
    }

    auto messageCallback = [this](const std::string &message)
    {
        Json::Value root;
        Json::Reader reader;

        if (reader.parse(message, root))
        {
            try
            {
                std::string previousHash = root["hash"].asString();
                double difficulty = root["difficulty"].asDouble();

                if (newBlockCallback_)
                {
                    newBlockCallback_(previousHash, difficulty);
                }

                // 生成并推送新的挖矿任务
                std::string newTask = generateTask(previousHash, difficulty);
                pushMiningTask(newTask);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error processing block message: " << e.what() << std::endl;
                return;
            }
        }
    };

    // 启动 Kafka 消费者
    if (!kafkaServer_.setupConsumer(blockTopic))
    {
        std::cerr << "Failed to setup block listener" << std::endl;
        return false;
    }

    isListening_ = true;
    return true;
}

void TaskGenerator::stopBlockListener()
{
}

void TaskGenerator::setNewBlockCallback(std::function<void(const std::string &, double)> callback)
{
    newBlockCallback_ = callback;
}