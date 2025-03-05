#ifndef TASK_GENERATOR_H
#define TASK_GENERATOR_H

#include "kafka_server.h"
#include <string>
#include <sqlite3.h>
#include "block_gen.h"

class TaskGenerator
{
public:
    TaskGenerator(const std::string &brokers, const std::string &topic);
    bool initDatabase();
    std::string TaskGenerator::generateCoinbaseTransaction();
    std::string generateTask(const std::string &previousHash, double difficultyTarget);
    bool storeTask(const std::string &jobId,
                   const std::string &coinbase,
                   const std::string &merkle,
                   const std::string &prevBlock,
                   const std::string &target);
    void pushMiningTask(const std::string &task);

private:
    KafkaServer kafkaServer_;
    sqlite3 *db_;
};

#endif // TASK_GENERATOR_H