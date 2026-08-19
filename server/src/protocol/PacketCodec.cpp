#include "protocol/PacketCodec.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace
{
std::uint32_t readUint32(const std::uint8_t* bytes)
{
    return (static_cast<std::uint32_t>(bytes[0]) << 24U) |
        (static_cast<std::uint32_t>(bytes[1]) << 16U) |
        (static_cast<std::uint32_t>(bytes[2]) << 8U) |
        static_cast<std::uint32_t>(bytes[3]);
}

void writeUint32(std::uint8_t* bytes, std::uint32_t value)
{
    bytes[0] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    bytes[1] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    bytes[2] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    bytes[3] = static_cast<std::uint8_t>(value & 0xFFU);
}
}

std::vector<std::uint8_t> PacketCodec::encode(
    std::uint32_t messageType,
    const im::protocol::v1::Envelope& envelope)
{
    const std::size_t bodySize = envelope.ByteSizeLong();
    if (bodySize == 0 || bodySize > kMaximumBodySize) {
        throw std::runtime_error("Protobuf body length is outside the allowed range");
    }
    std::vector<std::uint8_t> packet(kHeaderSize + bodySize);
    writeUint32(packet.data(), static_cast<std::uint32_t>(bodySize));
    writeUint32(packet.data() + 4, messageType);
    if (!envelope.SerializeToArray(packet.data() + kHeaderSize,
                                   static_cast<int>(bodySize))) {
        throw std::runtime_error("Failed to serialize Protobuf envelope");
    }
    return packet;
}

bool PacketCodec::decodeBody(const DecodedPacket& packet,
                             im::protocol::v1::Envelope& envelope)
{
    if (packet.body.empty() || packet.body.size() > kMaximumBodySize) {
        return false;
    }
    return envelope.ParseFromArray(packet.body.data(), static_cast<int>(packet.body.size()));
}

bool PacketCodec::isValidBodyLength(std::uint32_t length) noexcept
{
    return length > 0 && length <= kMaximumBodySize;
}

void PacketStreamDecoder::append(const std::uint8_t* data, std::size_t size)
{
    buffer_.insert(buffer_.end(), data, data + size);
}

PacketStreamDecoder::Result PacketStreamDecoder::next(DecodedPacket& packet)
{
    if (buffer_.size() < PacketCodec::kHeaderSize) {
        return Result::NeedMoreData;
    }
    const std::uint32_t bodyLength = readUint32(buffer_.data());
    if (!PacketCodec::isValidBodyLength(bodyLength)) {
        return Result::InvalidLength;
    }
    const std::size_t packetLength = PacketCodec::kHeaderSize + bodyLength;
    if (buffer_.size() < packetLength) {
        return Result::NeedMoreData;
    }
    packet.messageType = readUint32(buffer_.data() + 4);
    packet.body.assign(buffer_.begin() + PacketCodec::kHeaderSize,
                       buffer_.begin() + packetLength);
    buffer_.erase(buffer_.begin(), buffer_.begin() + packetLength);
    return Result::PacketReady;
}
