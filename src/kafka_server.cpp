#include "kafka_server.h"
#include <iostream>
#include <glog/logging.h>

KafkaServer::KafkaServer(const std::string &brokers, const std::string &topic)
    : brokers_(brokers), topic_(topic), producer_(nullptr), consumer_(nullptr), kafkaTopic_(nullptr)
{
    // 创建新的配置
    globalConf_ = rd_kafka_conf_new();
    topicConf_ = rd_kafka_topic_conf_new();

    // 设置 bootstrap.servers 配置项
    char errstr[512];
    if (rd_kafka_conf_set(globalConf_, "bootstrap.servers", brokers_.c_str(),
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
    {
        LOG(ERROR) << "Failed to set bootstrap.servers: " << errstr;
    }
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

    // 创建生产者
    rd_kafka_conf_t *conf = rd_kafka_conf_dup(globalConf_);
    producer_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
    if (!producer_)
    {
        LOG(ERROR) << "Failed to create producer: " << errstr;
        return false;
    }

    // bootstrap.servers 已在构造函数中设置，不再需要 rd_kafka_brokers_add

    // 创建主题
    kafkaTopic_ = rd_kafka_topic_new(producer_, topic_.c_str(), topicConf_);
    if (!kafkaTopic_)
    {
        LOG(ERROR) << "Failed to create topic: " << rd_kafka_err2str(rd_kafka_last_error());
        return false;
    }

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
        // LOG(INFO) << "Message sent: " << message;
    }

    rd_kafka_poll(producer_, 0); // Handle delivery reports
}

bool KafkaServer::setupConsumer(const std::string &topic)
{
    char errstr[512];

    rd_kafka_conf_t *conf = rd_kafka_conf_dup(globalConf_);
    if (!conf)
    {
        LOG(ERROR) << "Failed to duplicate config";
        return false;
    }

    // 设置必要的消费者配置
    const char *config_pairs[] = {
        "group.id", "mining_pool_group",
        "auto.offset.reset", "earliest",
        "enable.auto.commit", "true",
        "session.timeout.ms", "6000"};

    for (size_t i = 0; i < sizeof(config_pairs) / sizeof(*config_pairs); i += 2)
    {
        if (rd_kafka_conf_set(conf, config_pairs[i], config_pairs[i + 1],
                              errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
        {
            LOG(ERROR) << "Failed to set " << config_pairs[i] << ": " << errstr;
            rd_kafka_conf_destroy(conf);
            return false;
        }
    }

    // 创建消费者
    consumer_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
    if (!consumer_)
    {
        LOG(ERROR) << "Failed to create consumer: " << errstr;
        rd_kafka_conf_destroy(conf);
        return false;
    }

    // 配置消费者
    rd_kafka_poll_set_consumer(consumer_);

    // 订阅主题
    rd_kafka_topic_partition_list_t *topics = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(topics, topic.c_str(), RD_KAFKA_PARTITION_UA);

    rd_kafka_resp_err_t err = rd_kafka_subscribe(consumer_, topics);
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        LOG(ERROR) << "Failed to subscribe to topic: " << topic
                   << ", error: " << rd_kafka_err2str(err);
        rd_kafka_topic_partition_list_destroy(topics);
        rd_kafka_destroy(consumer_);
        consumer_ = nullptr;
        return false;
    }

    rd_kafka_topic_partition_list_destroy(topics);

    // 检查连接状态
    if (!checkConnection())
    {
        LOG(ERROR) << "Failed to establish connection to Kafka";
        rd_kafka_destroy(consumer_);
        consumer_ = nullptr;
        return false;
    }

    LOG(INFO) << "Successfully set up consumer for topic: " << topic;
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

    rd_kafka_t *handle = producer_ ? producer_ : consumer_;

    // 获取元数据来检查连接
    const struct rd_kafka_metadata *metadata;
    rd_kafka_resp_err_t err = rd_kafka_metadata(
        handle, 0, nullptr, &metadata, 5000);

    if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        LOG(ERROR) << "Failed to get metadata: " << rd_kafka_err2str(err);
        return false;
    }

    rd_kafka_metadata_destroy(metadata);
    return true;
}

void KafkaServer::setMessageCallback(std::function<void(const std::string &)> callback)
{
    messageCallback_ = callback;

    // 启动消息处理线程
    if (consumer_ && messageCallback_)
    {
        if (consumerThread_.joinable())
        {
            stopConsumer();
        }

        isRunning_ = true;
        consumerThread_ = std::thread([this]()
                                      {
            while (isRunning_) {
                std::string message = consumeMessage(100);
                if (!message.empty() && messageCallback_) {
                    messageCallback_(message);
                }
            } });
    }
}

void KafkaServer::stopConsumer()
{
    isRunning_ = false;
    if (consumerThread_.joinable())
    {
        consumerThread_.join();
    }
}
