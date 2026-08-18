#include "gateway/Session.h"

#include "protocol/FrameCodec.h"

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <iostream>
#include <utility>

Session::Session(boost::asio::ip::tcp::socket socket,
                 std::shared_ptr<const RequestHandler> requestHandler)
    : socket_(std::move(socket)),
      requestHandler_(std::move(requestHandler))
{
}

void Session::start()
{
    readHeader();
}

void Session::readHeader()
{
    auto self = shared_from_this();
    boost::asio::async_read(
        socket_, boost::asio::buffer(header_),
        [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close(error.message().c_str());
                return;
            }

            const std::uint32_t payloadLength = FrameCodec::decodeLength(self->header_);
            if (!FrameCodec::isValidLength(payloadLength)) {
                self->close("Invalid frame length");
                return;
            }
            self->readPayload(payloadLength);
        });
}

void Session::readPayload(std::uint32_t payloadLength)
{
    payload_.assign(payloadLength, 0);
    auto self = shared_from_this();
    boost::asio::async_read(
        socket_, boost::asio::buffer(payload_),
        [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close(error.message().c_str());
                return;
            }
            self->processPayload();
        });
}

void Session::processPayload()
{
    im::protocol::v1::Envelope request;
    if (!FrameCodec::decode(payload_, request)) {
        closeAfterWrite_ = true;
        enqueueResponse(RequestHandler::errorResponse(
            "", 1001, "Malformed Protobuf message"));
        return;
    }

    enqueueResponse(requestHandler_->handle(request));
    readHeader();
}

void Session::enqueueResponse(const im::protocol::v1::Envelope& response)
{
    const bool writeInProgress = !writeQueue_.empty();
    writeQueue_.push_back(FrameCodec::encode(response));
    if (!writeInProgress) {
        writeNext();
    }
}

void Session::writeNext()
{
    auto self = shared_from_this();
    boost::asio::async_write(
        socket_, boost::asio::buffer(writeQueue_.front()),
        [self](const boost::system::error_code& error, std::size_t) {
            if (error) {
                self->close(error.message().c_str());
                return;
            }
            self->writeQueue_.pop_front();
            if (!self->writeQueue_.empty()) {
                self->writeNext();
            } else if (self->closeAfterWrite_) {
                self->close("Malformed Protobuf message");
            }
        });
}

void Session::close(const char* reason)
{
    if (socket_.is_open()) {
        std::cerr << "Session closed: " << reason << '\n';
        boost::system::error_code ignored;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
    }
}
