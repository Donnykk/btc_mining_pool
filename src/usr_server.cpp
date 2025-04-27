#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <tcp_server.h>

TCPServer *g_server = nullptr;

void signalHandler(int signal)
{
    std::cout << "\033[33m[信号]\033[0m 收到信号: " << signal << ", 正在停止服务..." << std::endl;
    if (g_server)
    {
        g_server->stop();
    }
}

int main()
{
    // 注册信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try
    {
        int httpPort = 9090;
        std::cout << "\033[32m[启动]\033[0m HTTP API 服务启动" << std::endl;
        std::cout << "├── 监听端口: " << httpPort << std::endl;
        std::cout << "├── 数据库: mining_pool.db" << std::endl;
        std::cout << "├── API 端点:" << std::endl;
        std::cout << "│   ├── GET  /api/miner/status" << std::endl;
        std::cout << "│   └── POST /api/miner/submit" << std::endl;
        std::cout << "└── 等待请求..." << std::endl;

        // 初始化 TCP 服务器
        TCPServer server(httpPort);
        g_server = &server;

        // 启动服务
        if (!server.start())
        {
            std::cerr << "\033[31m[错误]\033[0m HTTP API 服务启动失败" << std::endl;
            return 1;
        }

        // 服务会在 start() 中一直运行，直到收到停止信号
        std::cout << "\033[32m[停止]\033[0m HTTP API 服务已停止" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\033[31m[错误]\033[0m HTTP API 服务初始化失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}