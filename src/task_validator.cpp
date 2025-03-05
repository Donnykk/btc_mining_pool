#include "task_validator.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <iostream>

TaskValidator::TaskValidator()
{
    // Open database
    int result = sqlite3_open("mining_pool.db", &db_);
    if (result != SQLITE_OK)
    {
        std::cerr << "Failed to open database." << std::endl;
    }
}

bool TaskValidator::validate(const std::string &workerName,
                             const std::string &jobId,
                             const std::string &extraNonce,
                             const std::string &ntime,
                             const std::string &nonce)
{
    std::string hash = calculateHash(extraNonce, ntime, nonce);
    std::string target;
    sqlite3_stmt *stmt = nullptr;
    const char *query = "SELECT Target FROM Job WHERE JobId = ?;";
    if (sqlite3_prepare_v2(db_, query, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, jobId.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            target = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        std::cerr << "Failed to prepare select statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    return hash < target;
}

std::string TaskValidator::calculateHash(const std::string &extraNonce, const std::string &ntime, const std::string &nonce)
{
    std::string data = extraNonce + ntime + nonce;

    // First SHA256
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash);

    // Second SHA256
    unsigned char doubleHash[SHA256_DIGEST_LENGTH];
    SHA256(hash, SHA256_DIGEST_LENGTH, doubleHash);

    // to Hex
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(doubleHash[i]);
    }
    return oss.str();
}
