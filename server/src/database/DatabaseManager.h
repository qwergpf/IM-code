#pragma once

#include "config/ConfigManager.h"

#include <mysql/mysql.h>

#include <memory>
#include <string>

class DatabaseManager final
{
public:
    explicit DatabaseManager(ServerConfig config);
    ~DatabaseManager() = default;

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    void connect();
    void verifyConnection();
    bool isHealthy(std::string& message) noexcept;
    const std::string& databaseName() const noexcept;

private:
    struct MysqlDeleter
    {
        void operator()(MYSQL* connection) const noexcept;
    };

    struct ResultDeleter
    {
        void operator()(MYSQL_RES* result) const noexcept;
    };

    ServerConfig config_;
    std::unique_ptr<MYSQL, MysqlDeleter> connection_;
};
