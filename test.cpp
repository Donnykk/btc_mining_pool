#include "btc_node.h"
#include "block_gen.h"
#include <iostream>

int main()
{
    // GetBlock RPC URL
    std::string rpcUrl = "https://go.getblock.io/fac1ed1394ed48e7b09e81bf7ddd4f86";

    BTC_Node bn;
    std::string payload = bn.createGetBlockPayload();
    std::string response = bn.sendJsonRpcRequest(rpcUrl, payload);
    bn.parseAndPrintBlockInfo(response);

    // Block Info
    std::string block_hash = bn.getBestBlockHash();
    int block_height = bn.getBlockHeight();
    double difficulty = bn.getDifficulty();

    BlockGenerator blockGenerator(block_hash, difficulty);
    BlockHeader newBlock = blockGenerator.generateBlock();

    std::cout << "\nNew Block Generated:\n";
    std::cout << "Previous Hash: " << newBlock.previousHash << "\n";
    std::cout << "Merkle Root: " << newBlock.merkleRoot << "\n";
    std::cout << "Timestamp: " << newBlock.timestamp << "\n";
    std::cout << "Nonce: " << newBlock.nonce << "\n";

    return 0;
}
