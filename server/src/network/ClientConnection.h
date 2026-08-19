#pragma once

#include "protocol/PacketCodec.h"
#include "protocol/ProtocolHandler.h"

#include <boost/asio/ip/tcp.hpp>

#include <array>
#include <deque>
#include <memory>
#include <string>
#include <vector>

class ClientConnection final : public std::enable_shared_from_this<ClientConnection>
{
public:
    ClientConnection(boost::asio::ip::tcp::socket socket,
                     std::shared_ptr<const ProtocolHandler> handler);

    void start();

private:
    void readSome();
    void processPackets();
    void enqueueResponse(std::uint32_t messageType,
                         const im::protocol::v1::Envelope& response);
    void writeNext();
    void close(const std::string& reason);

    boost::asio::ip::tcp::socket socket_;
    std::shared_ptr<const ProtocolHandler> handler_;
    std::array<std::uint8_t, 8192> readBuffer_{};
    PacketStreamDecoder decoder_;
    std::deque<std::vector<std::uint8_t>> writeQueue_;
    bool closing_{false};
    bool closeAfterWrite_{false};
};
