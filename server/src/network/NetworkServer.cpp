#include "network/NetworkServer.h"

#include "logging/Logger.h"
#include "network/ClientConnection.h"

#include <boost/asio/ip/address.hpp>

#include <utility>

NetworkServer::NetworkServer(boost::asio::io_context& ioContext,
                             std::string listenIp,
                             std::uint16_t listenPort,
                             std::shared_ptr<const ProtocolHandler> handler)
    : acceptor_(ioContext),
      handler_(std::move(handler))
{
    const auto address = boost::asio::ip::make_address(listenIp);
    const boost::asio::ip::tcp::endpoint endpoint(address, listenPort);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
    acceptNext();
}

void NetworkServer::acceptNext()
{
    acceptor_.async_accept(
        [this](const boost::system::error_code& error,
               boost::asio::ip::tcp::socket socket) {
            if (!error) {
                std::make_shared<ClientConnection>(std::move(socket), handler_)->start();
            } else if (acceptor_.is_open()) {
                Logger::error("Accept failed: " + error.message());
            }
            if (acceptor_.is_open()) {
                acceptNext();
            }
        });
}
