#ifndef KAFKA_SERVER_H
#define KAFKA_SERVER_H

#include <string>
#include <vector>
#include <map>
#include <librdkafka/rdkafka.h>

class KafkaServer
{
public:
    KafkaServer(const std::string &brokers, const std::string &topic);
    ~KafkaServer();

    // Kafka producer
    bool setupProducer();
    void sendMessage(const std::string &message);

    // Kafka consumer
    bool setupConsumer(int64_t offset = RD_KAFKA_OFFSET_END);
    std::string consumeMessage(int timeoutMs = 1000);

    // Utility
    bool checkConnection();

private:
    std::string brokers_;
    std::string topic_;
    rd_kafka_t *producer_;
    rd_kafka_t *consumer_;
    rd_kafka_topic_t *kafkaTopic_;
    rd_kafka_topic_conf_t *topicConf_;
    rd_kafka_conf_t *globalConf_;
};

#endif // KAFKA_SERVER_H