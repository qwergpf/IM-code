#include "config/ServerConfig.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <stdexcept>

TEST(ServerConfigTests, ParsesValidPort)
{
    EXPECT_EQ(ServerConfig::parsePort("PORT", "9000"), 9000);
    EXPECT_EQ(ServerConfig::parsePort("PORT", "65535"), 65535);
}

TEST(ServerConfigTests, RejectsInvalidPorts)
{
    EXPECT_THROW(ServerConfig::parsePort("PORT", "0"), std::runtime_error);
    EXPECT_THROW(ServerConfig::parsePort("PORT", "65536"), std::runtime_error);
    EXPECT_THROW(ServerConfig::parsePort("PORT", "not-a-port"), std::runtime_error);
    EXPECT_THROW(ServerConfig::parsePort("PORT", "9000x"), std::runtime_error);
}

TEST(ServerConfigTests, RequiresDatabasePassword)
{
    const char* existingPassword = std::getenv("IM_DB_PASSWORD");
    const std::string savedPassword =
        existingPassword == nullptr ? std::string() : std::string(existingPassword);

    unsetenv("IM_DB_PASSWORD");
    EXPECT_THROW(ServerConfig::fromEnvironment(), std::runtime_error);

    if (existingPassword != nullptr) {
        setenv("IM_DB_PASSWORD", savedPassword.c_str(), 1);
    }
}
