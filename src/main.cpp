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

    try
    {
        const std::string brokers = "localhost:9092";
        const std::string taskTopic = "mining_tasks";
        const std::string blockTopic = "new_blocks";

        std::cout << "正在连接 Kafka 服务器: " << brokers << std::endl;

        // Task Generator
        TaskGenerator taskGen(brokers, taskTopic);
        g_taskGen = &taskGen;
        if (!taskGen.startBlockListener(blockTopic))
        {
            std::cerr << "Failed to start block listener!" << std::endl;
            return 1;
        }
        std::cout << "Task Generator started!" << std::endl;

        // BTC Node
        BTC_Node btc_node;
        int pollInterval = 60;
        std::thread pollThread(&BTC_Node::startPoll, &btc_node, pollInterval);
        pollThread.detach();
        std::cout << "BTC Node Polling thread started!" << std::endl;

        // 启动 Stratum Server (矿工连接)
        int stratumPort = 3333;
        StratumServer stratumServer(stratumPort);
        g_server = &stratumServer;

        // 启动 HTTP Server (API 查询)
        int httpPort = 9090;
        TCPServer httpServer(httpPort);

        std::cout << "Starting Stratum Server on port " << stratumPort << std::endl;
        std::cout << "Starting HTTP Server on port " << httpPort << std::endl;

        // 启动两个服务器线程
        std::thread stratumThread([&stratumServer]()
                                  {
            if (!stratumServer.start()) {
                std::cerr << "Failed to start Stratum Server!" << std::endl;
            } });

        std::thread httpThread([&httpServer]()
                               {
            if (!httpServer.start()) {
                std::cerr << "Failed to start HTTP Server!" << std::endl;
            } });

        // 等待线程结束
        stratumThread.join();
        httpThread.join();
    }
    catch (const std::exception &e)
    {
        std::cerr << "初始化失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
