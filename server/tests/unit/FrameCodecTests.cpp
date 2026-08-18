#include "protocol/FrameCodec.h"

#include <gtest/gtest.h>

#include <algorithm>

TEST(FrameCodecTests, EncodesLengthInNetworkByteOrder)
{
    const auto header = FrameCodec::encodeLength(0x01020304U);
    EXPECT_EQ(header[0], 0x01);
    EXPECT_EQ(header[1], 0x02);
    EXPECT_EQ(header[2], 0x03);
    EXPECT_EQ(header[3], 0x04);
    EXPECT_EQ(FrameCodec::decodeLength(header), 0x01020304U);
}

TEST(FrameCodecTests, SerializesAndParsesEnvelope)
{
    im::protocol::v1::Envelope source;
    source.set_protocol_version(1);
    source.set_request_id("request-1");
    source.mutable_ping_request()->set_text("hello");

    const auto frame = FrameCodec::encode(source);
    ASSERT_GT(frame.size(), FrameCodec::kHeaderSize);

    std::array<std::uint8_t, FrameCodec::kHeaderSize> header{};
    std::copy_n(frame.begin(), FrameCodec::kHeaderSize, header.begin());
    const auto payloadLength = FrameCodec::decodeLength(header);
    EXPECT_EQ(payloadLength, frame.size() - FrameCodec::kHeaderSize);

    std::vector<std::uint8_t> payload(frame.begin() + FrameCodec::kHeaderSize,
                                      frame.end());
    im::protocol::v1::Envelope parsed;
    ASSERT_TRUE(FrameCodec::decode(payload, parsed));
    EXPECT_EQ(parsed.request_id(), "request-1");
    EXPECT_EQ(parsed.ping_request().text(), "hello");
}

TEST(FrameCodecTests, RejectsInvalidLengths)
{
    EXPECT_FALSE(FrameCodec::isValidLength(0));
    EXPECT_TRUE(FrameCodec::isValidLength(FrameCodec::kMaximumPayloadSize));
    EXPECT_FALSE(FrameCodec::isValidLength(FrameCodec::kMaximumPayloadSize + 1));
}

TEST(FrameCodecTests, RejectsMalformedProtobuf)
{
    const std::vector<std::uint8_t> payload{0xFF, 0xFF, 0xFF};
    im::protocol::v1::Envelope envelope;
    EXPECT_FALSE(FrameCodec::decode(payload, envelope));
}

TEST(FrameStreamDecoderTests, ReassemblesFrameFromMultipleChunks)
{
    im::protocol::v1::Envelope envelope;
    envelope.set_protocol_version(1);
    envelope.set_request_id("split-1");
    envelope.mutable_ping_request()->set_text("split");
    const auto frame = FrameCodec::encode(envelope);

    FrameStreamDecoder decoder;
    std::vector<std::uint8_t> payload;
    decoder.append(frame.data(), 2);
    EXPECT_EQ(decoder.next(payload), FrameStreamDecoder::Result::NeedMoreData);
    decoder.append(frame.data() + 2, 3);
    EXPECT_EQ(decoder.next(payload), FrameStreamDecoder::Result::NeedMoreData);
    decoder.append(frame.data() + 5, frame.size() - 5);
    EXPECT_EQ(decoder.next(payload), FrameStreamDecoder::Result::FrameReady);

    im::protocol::v1::Envelope parsed;
    ASSERT_TRUE(FrameCodec::decode(payload, parsed));
    EXPECT_EQ(parsed.request_id(), "split-1");
}

TEST(FrameStreamDecoderTests, ExtractsMultipleFramesFromOneChunk)
{
    im::protocol::v1::Envelope first;
    first.set_protocol_version(1);
    first.set_request_id("first");
    first.mutable_ping_request()->set_text("one");
    im::protocol::v1::Envelope second;
    second.set_protocol_version(1);
    second.set_request_id("second");
    second.mutable_ping_request()->set_text("two");

    auto combined = FrameCodec::encode(first);
    const auto secondFrame = FrameCodec::encode(second);
    combined.insert(combined.end(), secondFrame.begin(), secondFrame.end());

    FrameStreamDecoder decoder;
    decoder.append(combined.data(), combined.size());
    std::vector<std::uint8_t> payload;
    EXPECT_EQ(decoder.next(payload), FrameStreamDecoder::Result::FrameReady);
    im::protocol::v1::Envelope parsedFirst;
    ASSERT_TRUE(FrameCodec::decode(payload, parsedFirst));
    EXPECT_EQ(parsedFirst.request_id(), "first");

    EXPECT_EQ(decoder.next(payload), FrameStreamDecoder::Result::FrameReady);
    im::protocol::v1::Envelope parsedSecond;
    ASSERT_TRUE(FrameCodec::decode(payload, parsedSecond));
    EXPECT_EQ(parsedSecond.request_id(), "second");
    EXPECT_EQ(decoder.next(payload), FrameStreamDecoder::Result::NeedMoreData);
}

TEST(FrameStreamDecoderTests, RejectsInvalidHeaderLength)
{
    const auto invalidHeader = FrameCodec::encodeLength(0);
    FrameStreamDecoder decoder;
    decoder.append(invalidHeader.data(), invalidHeader.size());
    std::vector<std::uint8_t> payload;
    EXPECT_EQ(decoder.next(payload), FrameStreamDecoder::Result::InvalidLength);
}
