#include "protocol/RequestHandler.h"

#include <chrono>
#include <cstdint>
#include <utility>

namespace
{
constexpr std::uint32_t kProtocolVersion = 1;
constexpr std::uint32_t kUnsupportedProtocolVersion = 1002;
constexpr std::uint32_t kMissingRequestId = 1003;
constexpr std::uint32_t kUnsupportedMessageType = 1004;
}

RequestHandler::RequestHandler(std::string databaseName,
                               DatabaseHealthCheck healthCheck)
    : databaseName_(std::move(databaseName)),
      healthCheck_(std::move(healthCheck))
{
}

im::protocol::v1::Envelope RequestHandler::handle(
    const im::protocol::v1::Envelope& request) const
{
    if (request.protocol_version() != kProtocolVersion) {
        return errorResponse(request.request_id(), kUnsupportedProtocolVersion,
                             "Unsupported protocol version");
    }
    if (request.request_id().empty()) {
        return errorResponse("", kMissingRequestId, "request_id must not be empty");
    }

    im::protocol::v1::Envelope response;
    response.set_protocol_version(kProtocolVersion);
    response.set_request_id(request.request_id());

    if (request.has_ping_request()) {
        auto* ping = response.mutable_ping_response();
        ping->set_text(request.ping_request().text());
        const auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
        ping->set_server_time_unix_ms(now.time_since_epoch().count());
        return response;
    }

    if (request.has_database_health_request()) {
        std::string message;
        const bool healthy = healthCheck_(message);
        auto* health = response.mutable_database_health_response();
        health->set_healthy(healthy);
        health->set_database_name(databaseName_);
        health->set_message(message);
        return response;
    }

    return errorResponse(request.request_id(), kUnsupportedMessageType,
                         "Unsupported request message type");
}

im::protocol::v1::Envelope RequestHandler::errorResponse(
    const std::string& requestId,
    std::uint32_t code,
    const std::string& message)
{
    im::protocol::v1::Envelope response;
    response.set_protocol_version(kProtocolVersion);
    response.set_request_id(requestId);
    auto* error = response.mutable_error_response();
    error->set_code(code);
    error->set_message(message);
    return response;
}
