#include "config/ServerConfig.h"
#include "database/Database.h"
#include "gateway/TcpServer.h"
#include "protocol/RequestHandler.h"

#include <boost/asio/io_context.hpp>

#include <exception>
#include <iostream>
#include <memory>
#include <utility>

int main()
{
    try {
        const ServerConfig config = ServerConfig::fromEnvironment();
        auto database = std::make_shared<Database>(config);
        database->verifyConnection();

        auto requestHandler = std::make_shared<RequestHandler>(
            database->databaseName(),
            [database](std::string& message) {
                return database->isHealthy(message);
            });

        boost::asio::io_context ioContext;
        TcpServer server(ioContext, config.serverHost, config.serverPort,
                         std::move(requestHandler));

        std::cout << "IM server listening on " << config.serverHost << ':'
                  << config.serverPort << " using database "
                  << config.databaseName << '\n';
        ioContext.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "IM server startup failed: " << error.what() << '\n';
        return 1;
    }
}
