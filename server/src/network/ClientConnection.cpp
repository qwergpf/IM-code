#include "network/ClientConnection.h"

#include "logging/Logger.h"

#include <boost/asio/read.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/write.hpp>

#include <sstream>
#include <utility>

ClientConnection::ClientConnection(
    boost::asio::ip::tcp::socket socket,
    std::shared_ptr<const ProtocolHandler> handler)
    : socket_(std::move(socket)),
      handler_(std::move(handler))
{
}

void ClientConnection::start()
{
    std::ostringstream address;
    address << socket_.remote_endpoint();
    Logger::info("Client connected: " + address.str());
    readSome();
}

void ClientConnection::readSome()
{
    auto self = shared_from_this();
    socket_.async_read_some(boost::asio::buffer(readBuffer_),
        [self](const boost::system::error_code& error, std::size_t bytesRead) {
            if (error) {
                self->close(error == boost::asio::error::eof ? "client disconnected" : error.message());
                return;
            }
            self->decoder_.append(self->readBuffer_.data(), bytesRead);
            self->processPackets();
            if (!self->closing_ && !self->closeAfterWrite_) {
                self->readSome();
            }
        });
}

void ClientConnection::processPackets()
{
    DecodedPacket packet;
    while (!closing_) {
        const auto result = decoder_.next(packet);
        if (result == PacketStreamDecoder::Result::NeedMoreData) {
            return;
        }
        if (result == PacketStreamDecoder::Result::InvalidLength) {
            close("invalid packet body length");
            return;
        }

        im::protocol::v1::Envelope request;
        if (!PacketCodec::decodeBody(packet, request)) {
            closeAfterWrite_ = true;
            enqueueResponse(static_cast<std::uint32_t>(im::protocol::v1::MESSAGE_TYPE_ERROR_RESPONSE),
                            ProtocolHandler::errorResponse(
                                "", im::protocol::v1::ERROR_CODE_MALFORMED_PACKET,
                                "Malformed Protobuf packet"));
            return;
        }

        const auto response = handler_->handle(packet.messageType, request);
        const auto responseType = response.has_ping_response()
            ? im::protocol::v1::MESSAGE_TYPE_PING_RESPONSE
            : response.has_database_health_response()
                ? im::protocol::v1::MESSAGE_TYPE_DATABASE_HEALTH_RESPONSE
                : response.has_error_response()
                    ? im::protocol::v1::MESSAGE_TYPE_ERROR_RESPONSE
                    : im::protocol::v1::MESSAGE_TYPE_ERROR_RESPONSE;
        enqueueResponse(static_cast<std::uint32_t>(responseType), response);
    }
}

void ClientConnection::enqueueResponse(
    std::uint32_t messageType,
    const im::protocol::v1::Envelope& response)
{
    const bool writeInProgress = !writeQueue_.empty();
    writeQueue_.push_back(PacketCodec::encode(messageType, response));
    if (!writeInProgress) {
        writeNext();
    }
}

void ClientConnection::writeNext()
{
    auto self = shared_from_this();
    boost::asio::async_write(socket_, boost::asio::buffer(writeQueue_.front()),
        [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close(error.message());
                return;
            }
            self->writeQueue_.pop_front();
            if (!self->writeQueue_.empty()) {
                self->writeNext();
            } else if (self->closeAfterWrite_) {
                self->close("malformed Protobuf packet");
            }
        });
}

void ClientConnection::close(const std::string& reason)
{
    if (closing_) {
        return;
    }
    closing_ = true;
    Logger::info("Client disconnected: " + reason);
    boost::system::error_code ignored;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
}
