#include "config/ConfigManager.h"

#include <cstdlib>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace
{
std::string trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string requiredEnvironment(const char* name)
{
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        throw std::runtime_error(std::string(name) + " must be set and must not be empty");
    }
    return value;
}

std::string requiredValue(const std::unordered_map<std::string, std::string>& values,
                          const char* key)
{
    const auto iterator = values.find(key);
    if (iterator == values.end() || iterator->second.empty()) {
        throw std::runtime_error(std::string("Missing configuration key: ") + key);
    }
    return iterator->second;
}
}

ServerConfig ConfigManager::load(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open configuration file: " + path);
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const std::string normalized = trim(line);
        if (normalized.empty() || normalized.front() == '#') {
            continue;
        }
        const auto separator = normalized.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error("Invalid configuration line " + std::to_string(lineNumber));
        }
        const std::string key = trim(normalized.substr(0, separator));
        const std::string value = trim(normalized.substr(separator + 1));
        if (key.empty()) {
            throw std::runtime_error("Configuration key is empty on line " + std::to_string(lineNumber));
        }
        values[key] = value;
    }

    return ServerConfig{
        requiredValue(values, "listen_ip"),
        parsePort("listen_port", requiredValue(values, "listen_port")),
        requiredValue(values, "db_host"),
        parsePort("db_port", requiredValue(values, "db_port")),
        requiredValue(values, "db_username"),
        requiredEnvironment("IM_DB_PASSWORD"),
        requiredValue(values, "db_database")
    };
}

std::uint16_t ConfigManager::parsePort(const std::string& name, const std::string& value)
{
    std::size_t parsedCharacters = 0;
    unsigned long port = 0;
    try {
        port = std::stoul(value, &parsedCharacters);
    } catch (const std::exception&) {
        throw std::runtime_error(name + " must be an integer between 1 and 65535");
    }
    if (parsedCharacters != value.size() || port == 0 ||
        port > std::numeric_limits<std::uint16_t>::max()) {
        throw std::runtime_error(name + " must be an integer between 1 and 65535");
    }
    return static_cast<std::uint16_t>(port);
}
