#include "btc_node.h"
#include "block_gen.h"
#include <iostream>
#include "task_gen.h"
#include "stratum_server.h"

StratumServer *g_server = nullptr;

void signalHandler(int signal)
{
    std::cout << "Recv signal: " << signal << ", stopping..." << std::endl;
    if (g_server)
    {
        g_server->stop();
    }
    exit(signal);
}

int main()
{
    int port = 9090;
    StratumServer server(port);
    g_server = &server;

    std::cout << "Start Stratum Server. Listening: " << port << std::endl;
    if (!server.start())
    {
        std::cerr << "Failed to Start Server!" << std::endl;
        return 1;
    }

    // Kafka broker address and topic
    // std::string brokers = "localhost:9092";
    // std::string topic = "Mining Tasks";

    // // GetBlock RPC URL
    // std::string rpcUrl = "https://go.getblock.io/fac1ed1394ed48e7b09e81bf7ddd4f86";

    // BTC_Node bn;
    // std::string payload = bn.createGetBlockPayload();
    // std::string response = bn.sendJsonRpcRequest(rpcUrl, payload);
    // bn.parseAndPrintBlockInfo(response);

    // // Block Info
    // std::string block_hash = bn.getBestBlockHash();
    // int block_height = bn.getBlockHeight();
    // double difficulty = bn.getDifficulty();

    // // Generate Task
    // TaskGenerator tg(brokers, topic);
    // std::string task = tg.generateTask(block_hash, difficulty);
    // tg.pushMiningTask(task);
    // std::cout << "Mining task pushed to Kafka." << std::endl;

    // BlockGenerator blockGenerator(block_hash, difficulty);
    // BlockHeader newBlock = blockGenerator.generateBlock();

    // std::cout << "\nNew Block Generated:\n";
    // std::cout << "Previous Hash: " << newBlock.previousHash << "\n";
    // std::cout << "Merkle Root: " << newBlock.merkleRoot << "\n";
    // std::cout << "Timestamp: " << newBlock.timestamp << "\n";
    // std::cout << "Nonce: " << newBlock.nonce << "\n";

    // bn.startPoll(60);
    return 0;
}
