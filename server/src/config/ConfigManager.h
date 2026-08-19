#pragma once

#include <cstdint>
#include <string>

struct ServerConfig
{
    std::string listenIp;
    std::uint16_t listenPort{};
    std::string databaseHost;
    std::uint16_t databasePort{};
    std::string databaseUsername;
    std::string databasePassword;
    std::string databaseName;
};

class ConfigManager final
{
public:
    static ServerConfig load(const std::string& path);
    static std::uint16_t parsePort(const std::string& name, const std::string& value);
};
