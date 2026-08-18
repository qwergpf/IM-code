#include "gateway/TcpServer.h"

#include "gateway/Session.h"

#include <boost/asio/ip/address.hpp>

#include <iostream>
#include <utility>

TcpServer::TcpServer(boost::asio::io_context& ioContext,
                     const std::string& host,
                     std::uint16_t port,
                     std::shared_ptr<const RequestHandler> requestHandler)
    : acceptor_(ioContext),
      requestHandler_(std::move(requestHandler))
{
    const auto address = boost::asio::ip::make_address(host);
    const boost::asio::ip::tcp::endpoint endpoint(address, port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
    acceptNext();
}

void TcpServer::acceptNext()
{
    acceptor_.async_accept(
        [this](const boost::system::error_code& error,
               boost::asio::ip::tcp::socket socket) {
            if (!error) {
                std::make_shared<Session>(std::move(socket), requestHandler_)->start();
            } else {
                std::cerr << "Accept failed: " << error.message() << '\n';
            }
            if (acceptor_.is_open()) {
                acceptNext();
            }
        });
}
