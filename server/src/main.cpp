#include "config/ConfigManager.h"
#include "logging/Logger.h"
#include "server/ServerApplication.h"

#include <exception>
#include <string>

int main(int argc, char* argv[])
{
    try {
        std::string configPath = "server.conf";
        if (argc == 3 && std::string(argv[1]) == "--config") {
            configPath = argv[2];
        } else if (argc != 1) {
            Logger::error("Usage: server [--config path]");
            return 2;
        }

        const ServerConfig config = ConfigManager::load(configPath);
        return ServerApplication{}.run(config);
    } catch (const std::exception& error) {
        Logger::error(std::string("Server startup failed: ") + error.what());
        return 1;
    }
}
