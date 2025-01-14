#ifndef TASK_GENERATOR_H
#define TASK_GENERATOR_H

#include "kafka_server.h"
#include <string>
#include "block_gen.h"

class TaskGenerator
{
public:
    TaskGenerator(const std::string &brokers, const std::string &topic);
    std::string generateTask(const std::string &previousHash, double difficultyTarget);
    void pushMiningTask(const std::string &task);

private:
    KafkaServer kafkaServer_;
};

#endif // TASK_GENERATOR_H