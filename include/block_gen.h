#ifndef BLOCK_GEN_H
#define BLOCK_GEN_H

#include <iostream>
#include <vector>
#include <string>
#include <openssl/sha.h>
#include <curl/curl.h>
#include <ctime>
#include "kafka_server.h"

// Get transactions
std::vector<std::string> getTransactions();

// Calculate SHA256 hash
std::string sha256(const std::string &data);

// Calculate Merkle root
std::string calculateMerkleRoot(const std::vector<std::string> &transactions);

// Fetch the latest block hash
std::string getBestBlockHash();

struct BlockHeader
{
    std::string previousHash;
    std::string merkleRoot;
    uint32_t timestamp;
    uint32_t nonce;
    uint32_t difficultyTarget;
    std::string toString() const;
};

class BlockGenerator
{
public:
    BlockGenerator(const std::string &previousHash, const double difficulty);
    // BlockHeader generateBlock();

private:
    std::string previousHash;
    double difficulty;
};

#endif // BLOCK_GEN_H
