#ifndef TASK_GENERATOR_H
#define TASK_GENERATOR_H

#include "kafka_server.h"
#include <string>
#include <sqlite3.h>
#include "block_gen.h"
#include <functional>

class TaskGenerator
{
public:
    TaskGenerator(const std::string &brokers, const std::string &topic);
    bool initDatabase();
    std::string generateCoinbaseTransaction();
    std::string generateTask(const std::string &previousHash, double difficultyTarget);
    bool storeTask(const std::string &jobId,
                   const std::string &coinbase,
                   const std::string &merkle,
                   const std::string &prevBlock,
                   const std::string &target);
    void pushMiningTask(const std::string &task);
    bool startBlockListener(const std::string &blockTopic);
    void stopBlockListener();
    void setNewBlockCallback(std::function<void(const std::string &, double)> callback);

private:
    KafkaServer kafkaServer_;
    sqlite3 *db_;
    bool isListening_;
    std::function<void(const std::string &, double)> newBlockCallback_;
};

#endif // TASK_GENERATOR_H