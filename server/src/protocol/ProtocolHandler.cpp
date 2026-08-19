#include "protocol/ProtocolHandler.h"

#include "logging/Logger.h"

#include <chrono>
#include <string>
#include <utility>

namespace
{
constexpr std::uint32_t kProtocolVersion = 1;

im::protocol::v1::Envelope baseResponse(const std::string& requestId)
{
    im::protocol::v1::Envelope response;
    response.set_protocol_version(kProtocolVersion);
    response.set_request_id(requestId);
    return response;
}

bool typeMatches(std::uint32_t messageType, const im::protocol::v1::Envelope& request)
{
    using im::protocol::v1::MessageType;
    switch (static_cast<MessageType>(messageType)) {
    case MessageType::MESSAGE_TYPE_PING_REQUEST:
        return request.has_ping_request();
    case MessageType::MESSAGE_TYPE_DATABASE_HEALTH_REQUEST:
        return request.has_database_health_request();
    case MessageType::MESSAGE_TYPE_REGISTER_REQUEST:
        return request.has_register_request();
    case MessageType::MESSAGE_TYPE_LOGIN_REQUEST:
        return request.has_login_request();
    default:
        return false;
    }
}

bool isSupportedRequestType(std::uint32_t messageType)
{
    using im::protocol::v1::MessageType;
    switch (static_cast<MessageType>(messageType)) {
    case MessageType::MESSAGE_TYPE_PING_REQUEST:
    case MessageType::MESSAGE_TYPE_DATABASE_HEALTH_REQUEST:
    case MessageType::MESSAGE_TYPE_REGISTER_REQUEST:
    case MessageType::MESSAGE_TYPE_LOGIN_REQUEST:
        return true;
    default:
        return false;
    }
}
}

ProtocolHandler::ProtocolHandler(std::shared_ptr<DatabaseManager> database)
    : database_(std::move(database))
{
}

im::protocol::v1::Envelope ProtocolHandler::handle(
    std::uint32_t messageType,
    const im::protocol::v1::Envelope& request) const
{
    using im::protocol::v1::ErrorCode;
    using im::protocol::v1::MessageType;

    if (request.protocol_version() != kProtocolVersion) {
        return errorResponse(request.request_id(), ErrorCode::ERROR_CODE_UNSUPPORTED_PROTOCOL_VERSION,
                             "Unsupported protocol version");
    }
    if (request.request_id().empty()) {
        return errorResponse("", ErrorCode::ERROR_CODE_MISSING_REQUEST_ID,
                             "request_id must not be empty");
    }
    if (!isSupportedRequestType(messageType)) {
        return errorResponse(request.request_id(), ErrorCode::ERROR_CODE_UNSUPPORTED_MESSAGE_TYPE,
                             "Unsupported request message type");
    }
    if (!typeMatches(messageType, request)) {
        return errorResponse(request.request_id(), ErrorCode::ERROR_CODE_MESSAGE_TYPE_MISMATCH,
                             "Packet message type does not match its Protobuf payload");
    }

    im::protocol::v1::Envelope response = baseResponse(request.request_id());
    switch (static_cast<MessageType>(messageType)) {
    case MessageType::MESSAGE_TYPE_PING_REQUEST: {
        auto* ping = response.mutable_ping_response();
        ping->set_code(static_cast<std::uint32_t>(ErrorCode::ERROR_CODE_OK));
        ping->set_message("OK");
        ping->set_text(request.ping_request().text());
        const auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
        ping->set_server_time_unix_ms(now.time_since_epoch().count());
        return response;
    }
    case MessageType::MESSAGE_TYPE_DATABASE_HEALTH_REQUEST: {
        std::string message;
        const bool healthy = database_->isHealthy(message);
        auto* health = response.mutable_database_health_response();
        health->set_code(healthy ? static_cast<std::uint32_t>(ErrorCode::ERROR_CODE_OK)
                                 : static_cast<std::uint32_t>(ErrorCode::ERROR_CODE_DATABASE_UNAVAILABLE));
        health->set_message(message);
        health->set_healthy(healthy);
        health->set_database_name(database_->databaseName());
        return response;
    }
    case MessageType::MESSAGE_TYPE_REGISTER_REQUEST:
    case MessageType::MESSAGE_TYPE_LOGIN_REQUEST:
        return errorResponse(request.request_id(), ErrorCode::ERROR_CODE_NOT_IMPLEMENTED,
                             "This business operation is not implemented in phase 3");
    default:
        return errorResponse(request.request_id(), ErrorCode::ERROR_CODE_UNSUPPORTED_MESSAGE_TYPE,
                             "Unsupported request message type");
    }
}

im::protocol::v1::Envelope ProtocolHandler::errorResponse(
    const std::string& requestId,
    im::protocol::v1::ErrorCode code,
    const std::string& message)
{
    im::protocol::v1::Envelope response = baseResponse(requestId);
    auto* error = response.mutable_error_response();
    error->set_code(static_cast<std::uint32_t>(code));
    error->set_message(message);
    error->set_status(im::protocol::v1::RESPONSE_STATUS_ERROR);
    return response;
}
