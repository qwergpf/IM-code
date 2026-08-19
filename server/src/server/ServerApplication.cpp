#include "server/ServerApplication.h"

#include "database/DatabaseManager.h"
#include "logging/Logger.h"
#include "network/NetworkServer.h"
#include "protocol/ProtocolHandler.h"

#include <boost/asio/io_context.hpp>

#include <memory>
#include <string>

int ServerApplication::run(const ServerConfig& config)
{
    Logger::info("Server starting");

    auto database = std::make_shared<DatabaseManager>(config);
    Logger::info("Connecting to MySQL at " + config.databaseHost + ':' +
                 std::to_string(config.databasePort));
    database->connect();
    database->verifyConnection();
    Logger::info("MySQL connected successfully to database " + database->databaseName());

    auto handler = std::make_shared<ProtocolHandler>(database);
    boost::asio::io_context ioContext;
    NetworkServer server(ioContext, config.listenIp, config.listenPort, handler);
    Logger::info("Server listening on " + config.listenIp + ':' +
                 std::to_string(config.listenPort));
    ioContext.run();
    return 0;
}
