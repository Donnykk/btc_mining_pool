#include "kafka_server.h"
#include <iostream>
#include <glog/logging.h>

KafkaServer::KafkaServer(const std::string &brokers, const std::string &topic)
    : brokers_(brokers), topic_(topic), producer_(nullptr), consumer_(nullptr), kafkaTopic_(nullptr)
{
    globalConf_ = rd_kafka_conf_new();
    topicConf_ = rd_kafka_topic_conf_new();
}

KafkaServer::~KafkaServer()
{
    if (producer_)
    {
        rd_kafka_flush(producer_, 1000);
        rd_kafka_destroy(producer_);
    }
    if (consumer_)
    {
        rd_kafka_consumer_close(consumer_);
        rd_kafka_destroy(consumer_);
    }
    if (kafkaTopic_)
    {
        rd_kafka_topic_destroy(kafkaTopic_);
    }
    rd_kafka_conf_destroy(globalConf_);
    rd_kafka_topic_conf_destroy(topicConf_);
}

bool KafkaServer::setupProducer()
{
    char errstr[512];
    producer_ = rd_kafka_new(RD_KAFKA_PRODUCER, globalConf_, errstr, sizeof(errstr));
    if (!producer_)
    {
        LOG(ERROR) << "Failed to create producer: " << errstr;
        return false;
    }
    if (rd_kafka_brokers_add(producer_, brokers_.c_str()) == 0)
    {
        LOG(ERROR) << "Failed to add brokers: " << brokers_;
        return false;
    }
    kafkaTopic_ = rd_kafka_topic_new(producer_, topic_.c_str(), topicConf_);
    return true;
}

void KafkaServer::sendMessage(const std::string &message)
{
    if (!producer_)
    {
        LOG(ERROR) << "Producer not initialized.";
        return;
    }

    int err = rd_kafka_produce(
        kafkaTopic_, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY,
        (void *)message.c_str(), message.size(), nullptr, 0, nullptr);
    if (err == -1)
    {
        LOG(ERROR) << "Failed to produce message: " << rd_kafka_err2str(rd_kafka_last_error());
    }
    else
    {
        LOG(INFO) << "Message sent: " << message;
    }

    rd_kafka_poll(producer_, 0); // Handle delivery reports
}

bool KafkaServer::setupConsumer(int64_t offset)
{
    char errstr[512];
    rd_kafka_conf_set(globalConf_, "group.id", "mining_pool_group", errstr, sizeof(errstr));
    rd_kafka_conf_set(globalConf_, "auto.offset.reset", "earliest", nullptr, 0);

    consumer_ = rd_kafka_new(RD_KAFKA_CONSUMER, globalConf_, errstr, sizeof(errstr));
    if (!consumer_)
    {
        LOG(ERROR) << "Failed to create consumer: " << errstr;
        return false;
    }

    if (rd_kafka_brokers_add(consumer_, brokers_.c_str()) == 0)
    {
        LOG(ERROR) << "Failed to add brokers: " << brokers_;
        return false;
    }

    rd_kafka_poll_set_consumer(consumer_);

    rd_kafka_topic_partition_list_t *topics = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(topics, topic_.c_str(), RD_KAFKA_PARTITION_UA);

    if (rd_kafka_subscribe(consumer_, topics))
    {
        LOG(ERROR) << "Failed to subscribe to topic: " << topic_;
        return false;
    }

    rd_kafka_topic_partition_list_destroy(topics);
    return true;
}

std::string KafkaServer::consumeMessage(int timeoutMs)
{
    if (!consumer_)
    {
        LOG(ERROR) << "Consumer not initialized.";
        return "";
    }

    rd_kafka_message_t *msg = rd_kafka_consumer_poll(consumer_, timeoutMs);
    if (!msg)
    {
        return ""; // Timeout
    }

    std::string message;
    if (msg->err)
    {
        LOG(ERROR) << "Consume error: " << rd_kafka_message_errstr(msg);
    }
    else
    {
        message.assign((char *)msg->payload, msg->len);
    }

    rd_kafka_message_destroy(msg);
    return message;
}

bool KafkaServer::checkConnection()
{
    if (!producer_ && !consumer_)
    {
        return false;
    }
    return rd_kafka_brokers_add(producer_ ? producer_ : consumer_, brokers_.c_str()) > 0;
}