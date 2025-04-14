#include "ConfigManager.h"
#include <fstream>
#include <iostream>

ConfigManager &ConfigManager::getInstance()
{
    static ConfigManager instance;
    if (!instance.initialized_)
    {
        if (!instance.loadConfig())
        {
            std::cerr << "[ERROR] Failed to initialize ConfigManager with file: " << instance.filePath << std::endl;
            throw std::runtime_error("Configuration initialization failed");
        }
        instance.initialized_ = true;
    }
    return instance;
}

bool ConfigManager::loadConfig()
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] Cannot open config file: " << filePath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        // 跳过空行和注释
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        // 查找等号位置
        auto pos = line.find('=');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // 去除首尾空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            config_[key] = value;
        }
    }
    return true;
}

std::string ConfigManager::getValue(const std::string &key) const
{
    auto it = config_.find(key);
    if (it != config_.end())
    {
        return it->second;
    }
    return "";
}