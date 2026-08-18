#pragma once

#include <cstdint>
#include <string>

struct ServerConfig
{
    std::string serverHost;
    std::uint16_t serverPort;
    std::string databaseHost;
    std::uint16_t databasePort;
    std::string databaseName;
    std::string databaseUser;
    std::string databasePassword;

    static ServerConfig fromEnvironment();
    static std::uint16_t parsePort(const std::string& name, const std::string& value);
};
