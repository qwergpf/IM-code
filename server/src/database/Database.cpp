#include "database/Database.h"

#include <stdexcept>

namespace
{
std::string quoteConnectionValue(const std::string& value)
{
    std::string quoted = "'";
    for (const char character : value) {
        if (character == '\\' || character == '\'') {
            quoted.push_back('\\');
        }
        quoted.push_back(character);
    }
    quoted.push_back('\'');
    return quoted;
}

std::string connectionString(const ServerConfig& config)
{
    return "host=" + quoteConnectionValue(config.databaseHost) +
        " port=" + std::to_string(config.databasePort) +
        " dbname=" + quoteConnectionValue(config.databaseName) +
        " user=" + quoteConnectionValue(config.databaseUser) +
        " password=" + quoteConnectionValue(config.databasePassword);
}
}

Database::Database(const ServerConfig& config) try
    : databaseName_(config.databaseName),
      connection_(connectionString(config))
{
}
catch (const std::exception&)
{
    throw std::runtime_error("Could not connect to PostgreSQL");
}

void Database::verifyConnection()
{
    if (!connection_.is_open()) {
        throw std::runtime_error("PostgreSQL connection is not open");
    }

    pqxx::nontransaction transaction(connection_);
    const pqxx::result result = transaction.exec("SELECT 1");
    if (result.empty() || result[0][0].as<int>() != 1) {
        throw std::runtime_error("PostgreSQL health query returned an unexpected result");
    }
}

bool Database::isHealthy(std::string& message) noexcept
{
    try {
        verifyConnection();
        message = "PostgreSQL connection is healthy";
        return true;
    } catch (const std::exception&) {
        message = "PostgreSQL is unavailable";
        return false;
    }
}

const std::string& Database::databaseName() const noexcept
{
    return databaseName_;
}
