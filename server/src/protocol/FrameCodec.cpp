#include "protocol/FrameCodec.h"

#include <algorithm>
#include <stdexcept>
#include <string>

std::array<std::uint8_t, FrameCodec::kHeaderSize> FrameCodec::encodeLength(
    std::uint32_t length)
{
    return {
        static_cast<std::uint8_t>((length >> 24U) & 0xFFU),
        static_cast<std::uint8_t>((length >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((length >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(length & 0xFFU)
    };
}

std::uint32_t FrameCodec::decodeLength(
    const std::array<std::uint8_t, kHeaderSize>& header)
{
    return (static_cast<std::uint32_t>(header[0]) << 24U) |
        (static_cast<std::uint32_t>(header[1]) << 16U) |
        (static_cast<std::uint32_t>(header[2]) << 8U) |
        static_cast<std::uint32_t>(header[3]);
}

std::vector<std::uint8_t> FrameCodec::encode(
    const im::protocol::v1::Envelope& envelope)
{
    const std::size_t payloadSize = envelope.ByteSizeLong();
    if (payloadSize == 0 || payloadSize > kMaximumPayloadSize) {
        throw std::runtime_error("Protobuf payload length is outside the allowed range");
    }

    const auto header = encodeLength(static_cast<std::uint32_t>(payloadSize));
    std::vector<std::uint8_t> frame(kHeaderSize + payloadSize);
    std::copy(header.begin(), header.end(), frame.begin());

    if (!envelope.SerializeToArray(frame.data() + kHeaderSize,
                                   static_cast<int>(payloadSize))) {
        throw std::runtime_error("Failed to serialize Protobuf envelope");
    }
    return frame;
}

bool FrameCodec::decode(const std::vector<std::uint8_t>& payload,
                        im::protocol::v1::Envelope& envelope)
{
    if (payload.empty() || payload.size() > kMaximumPayloadSize) {
        return false;
    }
    return envelope.ParseFromArray(payload.data(), static_cast<int>(payload.size()));
}

bool FrameCodec::isValidLength(std::uint32_t length) noexcept
{
    return length > 0 && length <= kMaximumPayloadSize;
}

void FrameStreamDecoder::append(const std::uint8_t* data, std::size_t size)
{
    buffer_.insert(buffer_.end(), data, data + size);
}

FrameStreamDecoder::Result FrameStreamDecoder::next(
    std::vector<std::uint8_t>& payload)
{
    if (buffer_.size() < FrameCodec::kHeaderSize) {
        return Result::NeedMoreData;
    }

    std::array<std::uint8_t, FrameCodec::kHeaderSize> header{};
    std::copy_n(buffer_.begin(), FrameCodec::kHeaderSize, header.begin());
    const std::uint32_t payloadLength = FrameCodec::decodeLength(header);
    if (!FrameCodec::isValidLength(payloadLength)) {
        return Result::InvalidLength;
    }

    const std::size_t frameLength = FrameCodec::kHeaderSize + payloadLength;
    if (buffer_.size() < frameLength) {
        return Result::NeedMoreData;
    }

    payload.assign(buffer_.begin() + FrameCodec::kHeaderSize,
                   buffer_.begin() + frameLength);
    buffer_.erase(buffer_.begin(), buffer_.begin() + frameLength);
    return Result::FrameReady;
}
