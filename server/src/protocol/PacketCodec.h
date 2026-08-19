#pragma once

#include "im_protocol.pb.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct DecodedPacket
{
    std::uint32_t messageType{};
    std::vector<std::uint8_t> body;
};

class PacketCodec final
{
public:
    static constexpr std::size_t kHeaderSize = 8;
    static constexpr std::uint32_t kMaximumBodySize = 1024U * 1024U;

    static std::vector<std::uint8_t> encode(std::uint32_t messageType,
                                             const im::protocol::v1::Envelope& envelope);
    static bool decodeBody(const DecodedPacket& packet,
                           im::protocol::v1::Envelope& envelope);
    static bool isValidBodyLength(std::uint32_t length) noexcept;
};

class PacketStreamDecoder final
{
public:
    enum class Result
    {
        NeedMoreData,
        PacketReady,
        InvalidLength
    };

    void append(const std::uint8_t* data, std::size_t size);
    Result next(DecodedPacket& packet);

private:
    std::vector<std::uint8_t> buffer_;
};
