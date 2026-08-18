#pragma once

#include "protocol/RequestHandler.h"

#include <boost/asio/ip/tcp.hpp>

#include <array>
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::ip::tcp::socket socket,
            std::shared_ptr<const RequestHandler> requestHandler);

    void start();

private:
    void readHeader();
    void readPayload(std::uint32_t payloadLength);
    void processPayload();
    void enqueueResponse(const im::protocol::v1::Envelope& response);
    void writeNext();
    void close(const char* reason);

    boost::asio::ip::tcp::socket socket_;
    std::shared_ptr<const RequestHandler> requestHandler_;
    std::array<std::uint8_t, 4> header_{};
    std::vector<std::uint8_t> payload_;
    std::deque<std::vector<std::uint8_t>> writeQueue_;
    bool closeAfterWrite_{false};
};
