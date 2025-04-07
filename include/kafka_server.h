#ifndef KAFKA_SERVER_H
#define KAFKA_SERVER_H

#include <string>
#include <vector>
#include <map>
#include <thread>
#include <functional>
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
    bool setupConsumer(const std::string &topic); // 移除了 KafkaServer:: 前缀
    std::string consumeMessage(int timeoutMs = 1000);
    void stopConsumer();

    // Utility
    bool checkConnection();

    void setMessageCallback(std::function<void(const std::string &)> callback);


private:
    std::string brokers_;
    std::string topic_;
    rd_kafka_t *producer_;
    rd_kafka_t *consumer_;
    rd_kafka_topic_t *kafkaTopic_;
    rd_kafka_topic_conf_t *topicConf_;
    rd_kafka_conf_t *globalConf_;
    std::function<void(const std::string &)> messageCallback_;
    std::thread consumerThread_;
    bool isRunning_ = false;
};

#endif // KAFKA_SERVER_H