#include "database/DatabaseManager.h"

#include <stdexcept>
#include <utility>

DatabaseManager::DatabaseManager(ServerConfig config)
    : config_(std::move(config))
{
}

void DatabaseManager::MysqlDeleter::operator()(MYSQL* connection) const noexcept
{
    if (connection != nullptr) {
        mysql_close(connection);
    }
}

void DatabaseManager::ResultDeleter::operator()(MYSQL_RES* result) const noexcept
{
    if (result != nullptr) {
        mysql_free_result(result);
    }
}

void DatabaseManager::connect()
{
    MYSQL* rawConnection = mysql_init(nullptr);
    if (rawConnection == nullptr) {
        throw std::runtime_error("MySQL initialization failed");
    }

    std::unique_ptr<MYSQL, MysqlDeleter> connection(rawConnection);
    unsigned int timeoutSeconds = 5;
    mysql_options(connection.get(), MYSQL_OPT_CONNECT_TIMEOUT, &timeoutSeconds);

    if (mysql_real_connect(connection.get(), config_.databaseHost.c_str(),
                            config_.databaseUsername.c_str(), config_.databasePassword.c_str(),
                            config_.databaseName.c_str(), config_.databasePort, nullptr, 0) == nullptr) {
        throw std::runtime_error("MySQL connection failed: " +
                                 std::string(mysql_error(connection.get())));
    }

    if (mysql_set_character_set(connection.get(), "utf8mb4") != 0) {
        throw std::runtime_error("MySQL character set setup failed: " +
                                 std::string(mysql_error(connection.get())));
    }
    connection_ = std::move(connection);
}

void DatabaseManager::verifyConnection()
{
    if (!connection_) {
        throw std::runtime_error("MySQL connection is not open");
    }
    if (mysql_query(connection_.get(), "SELECT 1") != 0) {
        throw std::runtime_error("MySQL health query failed: " +
                                 std::string(mysql_error(connection_.get())));
    }

    std::unique_ptr<MYSQL_RES, ResultDeleter> result(mysql_store_result(connection_.get()));
    if (!result) {
        throw std::runtime_error("MySQL health query returned no result");
    }
    MYSQL_ROW row = mysql_fetch_row(result.get());
    const bool valid = row != nullptr && row[0] != nullptr && std::string(row[0]) == "1";
    if (!valid) {
        throw std::runtime_error("MySQL health query returned an unexpected result");
    }
}

bool DatabaseManager::isHealthy(std::string& message) noexcept
{
    try {
        verifyConnection();
        message = "MySQL connection is healthy";
        return true;
    } catch (const std::exception&) {
        message = "MySQL is unavailable";
        return false;
    }
}

const std::string& DatabaseManager::databaseName() const noexcept
{
    return config_.databaseName;
}
