#pragma once

#include "im_protocol.pb.h"

#include <cstdint>
#include <functional>
#include <string>

class RequestHandler
{
public:
    using DatabaseHealthCheck = std::function<bool(std::string&)>;

    RequestHandler(std::string databaseName, DatabaseHealthCheck healthCheck);

    im::protocol::v1::Envelope handle(
        const im::protocol::v1::Envelope& request) const;

    static im::protocol::v1::Envelope errorResponse(
        const std::string& requestId,
        std::uint32_t code,
        const std::string& message);

private:
    std::string databaseName_;
    DatabaseHealthCheck healthCheck_;
};
