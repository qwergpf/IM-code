#pragma once

#include "im_protocol.pb.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class FrameCodec
{
public:
    static constexpr std::size_t kHeaderSize = 4;
    static constexpr std::uint32_t kMaximumPayloadSize = 1024U * 1024U;

    static std::array<std::uint8_t, kHeaderSize> encodeLength(std::uint32_t length);
    static std::uint32_t decodeLength(const std::array<std::uint8_t, kHeaderSize>& header);
    static std::vector<std::uint8_t> encode(const im::protocol::v1::Envelope& envelope);
    static bool decode(const std::vector<std::uint8_t>& payload,
                       im::protocol::v1::Envelope& envelope);
    static bool isValidLength(std::uint32_t length) noexcept;
};

class FrameStreamDecoder
{
public:
    enum class Result
    {
        NeedMoreData,
        FrameReady,
        InvalidLength
    };

    void append(const std::uint8_t* data, std::size_t size);
    Result next(std::vector<std::uint8_t>& payload);

private:
    std::vector<std::uint8_t> buffer_;
};
