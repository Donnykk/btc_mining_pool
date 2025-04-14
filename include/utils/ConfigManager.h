#pragma once
#include <string>
#include <unordered_map>

class ConfigManager
{
public:
    static ConfigManager &getInstance();
    std::string getValue(const std::string &key) const;
    bool loadConfig();
    bool isInitialized() const { return initialized_; }

private:
    ConfigManager() = default;
    const std::string filePath = ".env";
    bool initialized_ = false;
    std::unordered_map<std::string, std::string> config_;
};