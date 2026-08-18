#pragma once

#include "config/ServerConfig.h"

#include <pqxx/pqxx>

#include <string>

class Database
{
public:
    explicit Database(const ServerConfig& config);

    void verifyConnection();
    bool isHealthy(std::string& message) noexcept;
    const std::string& databaseName() const noexcept;

private:
    std::string databaseName_;
    pqxx::connection connection_;
};
