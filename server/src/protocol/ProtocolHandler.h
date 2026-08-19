#pragma once

#include "database/DatabaseManager.h"
#include "im_protocol.pb.h"

#include <cstdint>
#include <memory>
#include <string>

class ProtocolHandler final
{
public:
    explicit ProtocolHandler(std::shared_ptr<DatabaseManager> database);

    im::protocol::v1::Envelope handle(
        std::uint32_t messageType,
        const im::protocol::v1::Envelope& request) const;

    static im::protocol::v1::Envelope errorResponse(
        const std::string& requestId,
        im::protocol::v1::ErrorCode code,
        const std::string& message);

private:
    std::shared_ptr<DatabaseManager> database_;
};
