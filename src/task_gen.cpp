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

    // 1. 交易版本号 (4 bytes)
    oss << "01000000";

    // 2. 输入数量 (1 byte)
    oss << "01";

    // 3. 输入
    // 3.1 空的交易ID (32 bytes)
    oss << "0000000000000000000000000000000000000000000000000000000000000000";
    // 3.2 输出索引 (4 bytes)
    oss << "ffffffff";

    // 3.3 coinbase 脚本
    std::string scriptSig = "03" + toHexString(time(nullptr), 32); // 只包含时间戳
    oss << toHexString(scriptSig.length() / 2, 8) << scriptSig;

    // 3.4 序列号 (4 bytes)
    oss << "ffffffff";

    // 4. 输出数量 (1 byte)
    oss << "01";

    // 5. 输出
    // 5.1 比特币数量 (8 bytes) - 50 BTC in satoshis
    oss << "0000e1f505000000";

    // 5.2 输出脚本
    std::string pubKeyScript = std::string("76a914") + "0000000000000000000000000000000000000000" + "88ac";
    oss << toHexString(pubKeyScript.length() / 2, 8) << pubKeyScript;

    // 6. 锁定时间 (4 bytes)
    oss << "00000000";

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

    std::cout << "Starting block listener for topic: " << blockTopic << std::endl;

    auto messageCallback = [this](const std::string &message)
    {
        // std::cout << "Received message from Kafka: " << message << std::endl;

        Json::Value root;
        Json::CharReaderBuilder reader;
        std::string errs;
        std::istringstream stream(message);

        if (!Json::parseFromStream(reader, stream, &root, &errs))
        {
            std::cerr << "Error parsing JSON: " << errs << std::endl;
            return;
        }

        try
        {
            std::string previousHash = root["hash"].asString();
            double difficulty = root["difficulty"].asDouble();

            std::cout << "Parsed block info - Hash: " << previousHash << ", Difficulty: " << difficulty << std::endl;

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
    };

    // 启动 Kafka 消费者，确保从最新消息开始消费
    if (!kafkaServer_.setupConsumer(blockTopic))
    {
        std::cerr << "Failed to setup block listener" << std::endl;
        return false;
    }

    // 设置消息回调
    kafkaServer_.setMessageCallback(messageCallback);

    isListening_ = true;
    return true;
}

void TaskGenerator::stopBlockListener()
{
    if (isListening_)
    {
        kafkaServer_.stopConsumer();
        isListening_ = false;
        std::cout << "区块监听器已停止" << std::endl;
    }
}

void TaskGenerator::setNewBlockCallback(std::function<void(const std::string &, double)> callback)
{
    newBlockCallback_ = callback;
}

TaskGenerator *g_taskGen = nullptr;

void signalHandler(int signal)
{
    std::cout << "接收到信号: " << signal << ", 正在停止..." << std::endl;
    if (g_taskGen)
    {
        g_taskGen->stopBlockListener();
    }
    exit(signal);
}

int main()
{
    // 注册信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try
    {
        // Kafka 配置
        const std::string brokers = "localhost:9092";
        const std::string taskTopic = "mining_tasks";
        const std::string blockTopic = "BTC_blocks";

        std::cout << "正在连接 Kafka 服务器: " << brokers << std::endl;

        // 初始化任务生成器
        TaskGenerator taskGen(brokers, taskTopic);
        g_taskGen = &taskGen;

        // 启动区块监听
        if (!taskGen.startBlockListener(blockTopic))
        {
            std::cerr << "启动区块监听失败！" << std::endl;
            return 1;
        }
        std::cout << "任务生成器启动成功！" << std::endl;
        std::cout << "监听区块主题: " << blockTopic << std::endl;
        std::cout << "发送任务主题: " << taskTopic << std::endl;

        // 保持程序运行
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "任务生成器初始化失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}