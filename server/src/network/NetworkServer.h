#pragma once

#include "protocol/ProtocolHandler.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <cstdint>
#include <memory>
#include <string>

class NetworkServer final
{
public:
    NetworkServer(boost::asio::io_context& ioContext,
                  std::string listenIp,
                  std::uint16_t listenPort,
                  std::shared_ptr<const ProtocolHandler> handler);

private:
    void acceptNext();

    boost::asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<const ProtocolHandler> handler_;
};
