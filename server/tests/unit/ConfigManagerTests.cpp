#include "config/ConfigManager.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>

namespace
{
class EnvironmentGuard
{
public:
    EnvironmentGuard()
    {
        const char* value = std::getenv("IM_DB_PASSWORD");
        if (value != nullptr) {
            previous_ = value;
        }
    }

    ~EnvironmentGuard()
    {
#ifdef _WIN32
        if (previous_.has_value()) {
            _putenv_s("IM_DB_PASSWORD", previous_->c_str());
        } else {
            _putenv_s("IM_DB_PASSWORD", "");
        }
#else
        if (previous_.has_value()) {
            setenv("IM_DB_PASSWORD", previous_->c_str(), 1);
        } else {
            unsetenv("IM_DB_PASSWORD");
        }
#endif
    }

private:
    std::optional<std::string> previous_;
};
}

TEST(ConfigManagerTests, LoadsConfigAndEnvironmentPassword)
{
    EnvironmentGuard guard;
#ifdef _WIN32
    _putenv_s("IM_DB_PASSWORD", "test-password");
#else
    setenv("IM_DB_PASSWORD", "test-password", 1);
#endif
    const std::string path = "config-manager-test.conf";
    std::ofstream(path) << "listen_ip=127.0.0.1\nlisten_port=8888\n"
                            "db_host=127.0.0.1\ndb_port=3306\n"
                            "db_username=im_app\ndb_database=im_chat\n";
    const auto config = ConfigManager::load(path);
    EXPECT_EQ(config.listenPort, 8888);
    EXPECT_EQ(config.databasePort, 3306);
    EXPECT_EQ(config.databasePassword, "test-password");
    std::remove(path.c_str());
}

TEST(ConfigManagerTests, RejectsInvalidPort)
{
    EXPECT_THROW(ConfigManager::parsePort("port", "70000"), std::runtime_error);
}

TEST(ConfigManagerTests, RejectsMissingFile)
{
    EXPECT_THROW(ConfigManager::load("missing-config-file.conf"), std::runtime_error);
}

TEST(ConfigManagerTests, RejectsEmptyPassword)
{
    EnvironmentGuard guard;
#ifdef _WIN32
    _putenv_s("IM_DB_PASSWORD", "");
#else
    unsetenv("IM_DB_PASSWORD");
#endif
    const std::string path = "config-empty-password-test.conf";
    std::ofstream(path) << "listen_ip=127.0.0.1\nlisten_port=8888\n"
                            "db_host=127.0.0.1\ndb_port=3306\n"
                            "db_username=im_app\ndb_database=im_chat\n";
    EXPECT_THROW(ConfigManager::load(path), std::runtime_error);
    std::remove(path.c_str());
}
