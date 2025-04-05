#include "btc_node.h"
#include "block_gen.h"
#include <iostream>
#include "task_gen.h"
#include "stratum_server.h"
#include <signal.h>

StratumServer *g_server = nullptr;
TaskGenerator *g_taskGen = nullptr;

void signalHandler(int signal)
{
    std::cout << "Recv signal: " << signal << ", stopping..." << std::endl;
    if (g_server)
    {
        g_server->stop();
    }
    if (g_taskGen)
    {
        g_taskGen->stopBlockListener();
    }
    exit(signal);
}

int main()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 初始化 Task Generator
    const std::string brokers = "localhost:9092";
    const std::string taskTopic = "mining_tasks";
    const std::string blockTopic = "new_blocks";

    std::cout << "正在连接 Kafka 服务器: " << brokers << std::endl;

    try
    {
        TaskGenerator taskGen(brokers, taskTopic);
        g_taskGen = &taskGen;
        if (!taskGen.startBlockListener(blockTopic))
        {
            std::cerr << "Failed to start block listener!" << std::endl;
            return 1;
        }
        std::cout << "Task Generator started!" << std::endl;

        // 启动 BTC Node
        BTC_Node btc_node;
        int pollInterval = 10;
        std::thread pollThread(&BTC_Node::startPoll, &btc_node, pollInterval);
        pollThread.detach();
        std::cout << "BTC Node Polling thread started!" << std::endl;

        // 启动 Stratum Server
        // int port = 9090;
        // StratumServer server(port);
        // g_server = &server;

        // std::cout << "Start Stratum Server. Listening: " << port << std::endl;
        // if (!server.start())
        // {
        //     std::cerr << "Failed to Start Server!" << std::endl;
        //     return 1;
        // }

        // 主线程等待
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "初始化失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
