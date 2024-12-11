#include "btc_node.h"
#include <iostream>

int main()
{
    // GetBlock RPC URL
    std::string rpcUrl = "https://go.getblock.io/fac1ed1394ed48e7b09e81bf7ddd4f86";

    // Create JSON-RPC payload
    std::string payload = createGetBlockPayload();

    // Send request and get response
    std::string response = sendJsonRpcRequest(rpcUrl, payload);

    // Parse and print block information
    parseAndPrintBlockInfo(response);

    // std::string previousHash = getBestBlockHash();

    // BlockGenerator blockGenerator(previousHash);
    // BlockHeader newBlock = blockGenerator.generateBlock();

    // std::cout << "\nNew Block Generated:\n";
    // std::cout << "Previous Hash: " << newBlock.previousHash << "\n";
    // std::cout << "Merkle Root: " << newBlock.merkleRoot << "\n";
    // std::cout << "Timestamp: " << newBlock.timestamp << "\n";
    // std::cout << "Nonce: " << newBlock.nonce << "\n";

    return 0;
}
