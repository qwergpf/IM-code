#include "config/ServerConfig.h"

#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace
{
std::string environmentValue(const char* name, const char* defaultValue)
{
    const char* value = std::getenv(name);
    return value == nullptr ? std::string(defaultValue) : std::string(value);
}
}

ServerConfig ServerConfig::fromEnvironment()
{
    const std::string password = environmentValue("IM_DB_PASSWORD", "");
    if (password.empty()) {
        throw std::runtime_error("IM_DB_PASSWORD must be set and must not be empty");
    }

    return ServerConfig{
        environmentValue("IM_SERVER_HOST", "0.0.0.0"),
        parsePort("IM_SERVER_PORT", environmentValue("IM_SERVER_PORT", "9000")),
        environmentValue("IM_DB_HOST", "127.0.0.1"),
        parsePort("IM_DB_PORT", environmentValue("IM_DB_PORT", "5432")),
        environmentValue("IM_DB_NAME", "im_chat"),
        environmentValue("IM_DB_USER", "im_app"),
        password
    };
}

std::uint16_t ServerConfig::parsePort(const std::string& name, const std::string& value)
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
