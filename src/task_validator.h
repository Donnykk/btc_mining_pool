#ifndef TASK_VALIDATOR_H
#define TASK_VALIDATOR_H
#include <sqlite3.h>
#include <string>

class TaskValidator
{
public:
    TaskValidator();
    bool validate(const std::string &workerName,
                  const std::string &jobId,
                  const std::string &extraNonce,
                  const std::string &ntime,
                  const std::string &nonce);

private:
    sqlite3 *db_;
    std::string calculateHash(const std::string &extraNonce, const std::string &ntime, const std::string &nonce);
};

#endif // TASK_VALIDATOR_H