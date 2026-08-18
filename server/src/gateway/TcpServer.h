#pragma once

#include "protocol/RequestHandler.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <cstdint>
#include <memory>
#include <string>

class TcpServer
{
public:
    TcpServer(boost::asio::io_context& ioContext,
              const std::string& host,
              std::uint16_t port,
              std::shared_ptr<const RequestHandler> requestHandler);

private:
    void acceptNext();

    boost::asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<const RequestHandler> requestHandler_;
};
