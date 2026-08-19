#include "protocol/PacketCodec.h"

#include <gtest/gtest.h>

TEST(PacketCodecTests, EncodesNetworkOrderHeaderAndRoundTrips)
{
    im::protocol::v1::Envelope source;
    source.set_protocol_version(1);
    source.set_request_id("ping-1");
    source.mutable_ping_request()->set_text("hello");

    const auto packet = PacketCodec::encode(
        im::protocol::v1::MESSAGE_TYPE_PING_REQUEST, source);
    ASSERT_GT(packet.size(), PacketCodec::kHeaderSize);
    EXPECT_EQ(packet[4], 0U);
    EXPECT_EQ(packet[5], 0U);
    EXPECT_EQ(packet[6], 0U);
    EXPECT_EQ(packet[7], 1U);

    PacketStreamDecoder decoder;
    DecodedPacket decoded;
    decoder.append(packet.data(), 3);
    EXPECT_EQ(decoder.next(decoded), PacketStreamDecoder::Result::NeedMoreData);
    decoder.append(packet.data() + 3, packet.size() - 3);
    ASSERT_EQ(decoder.next(decoded), PacketStreamDecoder::Result::PacketReady);
    im::protocol::v1::Envelope parsed;
    ASSERT_TRUE(PacketCodec::decodeBody(decoded, parsed));
    EXPECT_EQ(parsed.request_id(), "ping-1");
}

TEST(PacketCodecTests, ExtractsTwoConcatenatedPackets)
{
    im::protocol::v1::Envelope first;
    first.set_protocol_version(1);
    first.set_request_id("first");
    first.mutable_ping_request()->set_text("one");
    im::protocol::v1::Envelope second;
    second.set_protocol_version(1);
    second.set_request_id("second");
    second.mutable_ping_request()->set_text("two");

    auto combined = PacketCodec::encode(im::protocol::v1::MESSAGE_TYPE_PING_REQUEST, first);
    const auto secondPacket = PacketCodec::encode(im::protocol::v1::MESSAGE_TYPE_PING_REQUEST, second);
    combined.insert(combined.end(), secondPacket.begin(), secondPacket.end());

    PacketStreamDecoder decoder;
    decoder.append(combined.data(), combined.size());
    DecodedPacket decoded;
    ASSERT_EQ(decoder.next(decoded), PacketStreamDecoder::Result::PacketReady);
    ASSERT_EQ(decoder.next(decoded), PacketStreamDecoder::Result::PacketReady);
    EXPECT_EQ(decoder.next(decoded), PacketStreamDecoder::Result::NeedMoreData);
}

TEST(PacketCodecTests, RejectsInvalidLengths)
{
    PacketStreamDecoder decoder;
    const std::uint8_t zeroHeader[PacketCodec::kHeaderSize]{};
    decoder.append(zeroHeader, sizeof(zeroHeader));
    DecodedPacket packet;
    EXPECT_EQ(decoder.next(packet), PacketStreamDecoder::Result::InvalidLength);
    EXPECT_FALSE(PacketCodec::isValidBodyLength(PacketCodec::kMaximumBodySize + 1));

    const std::uint32_t tooLarge = PacketCodec::kMaximumBodySize + 1;
    const std::uint8_t oversizedHeader[PacketCodec::kHeaderSize] = {
        static_cast<std::uint8_t>((tooLarge >> 24U) & 0xFFU),
        static_cast<std::uint8_t>((tooLarge >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((tooLarge >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(tooLarge & 0xFFU),
        0, 0, 0, 1
    };
    PacketStreamDecoder oversizedDecoder;
    oversizedDecoder.append(oversizedHeader, sizeof(oversizedHeader));
    EXPECT_EQ(oversizedDecoder.next(packet), PacketStreamDecoder::Result::InvalidLength);
}

TEST(PacketCodecTests, RejectsMalformedProtobufBody)
{
    DecodedPacket packet;
    packet.messageType = im::protocol::v1::MESSAGE_TYPE_PING_REQUEST;
    packet.body = {0xFF, 0xFF, 0xFF};
    im::protocol::v1::Envelope envelope;
    EXPECT_FALSE(PacketCodec::decodeBody(packet, envelope));
}
