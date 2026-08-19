#include "protocol/ProtocolHandler.h"

#include <gtest/gtest.h>

TEST(ProtocolHandlerTests, PingPreservesRequestIdAndEchoesText)
{
    ProtocolHandler handler(nullptr);
    im::protocol::v1::Envelope request;
    request.set_protocol_version(1);
    request.set_request_id("request-1");
    request.mutable_ping_request()->set_text("hello");

    const auto response = handler.handle(im::protocol::v1::MESSAGE_TYPE_PING_REQUEST, request);
    EXPECT_EQ(response.request_id(), "request-1");
    ASSERT_TRUE(response.has_ping_response());
    EXPECT_EQ(response.ping_response().text(), "hello");
    EXPECT_EQ(response.ping_response().code(), 0U);
}

TEST(ProtocolHandlerTests, RejectsMissingRequestIdAndUnsupportedVersion)
{
    ProtocolHandler handler(nullptr);
    im::protocol::v1::Envelope request;
    request.set_protocol_version(1);
    request.mutable_ping_request()->set_text("hello");
    auto response = handler.handle(im::protocol::v1::MESSAGE_TYPE_PING_REQUEST, request);
    ASSERT_TRUE(response.has_error_response());
    EXPECT_EQ(response.error_response().code(),
              im::protocol::v1::ERROR_CODE_MISSING_REQUEST_ID);

    request.set_request_id("version");
    request.set_protocol_version(99);
    response = handler.handle(im::protocol::v1::MESSAGE_TYPE_PING_REQUEST, request);
    EXPECT_EQ(response.error_response().code(),
              im::protocol::v1::ERROR_CODE_UNSUPPORTED_PROTOCOL_VERSION);
}

TEST(ProtocolHandlerTests, LoginIsExplicitlyNotImplemented)
{
    ProtocolHandler handler(nullptr);
    im::protocol::v1::Envelope request;
    request.set_protocol_version(1);
    request.set_request_id("login-1");
    request.mutable_login_request()->set_username("demo");
    const auto response = handler.handle(im::protocol::v1::MESSAGE_TYPE_LOGIN_REQUEST, request);
    ASSERT_TRUE(response.has_error_response());
    EXPECT_EQ(response.error_response().code(), im::protocol::v1::ERROR_CODE_NOT_IMPLEMENTED);
}

TEST(ProtocolHandlerTests, RejectsUnknownAndMismatchedMessageTypes)
{
    ProtocolHandler handler(nullptr);
    im::protocol::v1::Envelope request;
    request.set_protocol_version(1);
    request.set_request_id("type-1");
    request.mutable_ping_request()->set_text("hello");

    auto response = handler.handle(999U, request);
    ASSERT_TRUE(response.has_error_response());
    EXPECT_EQ(response.error_response().code(),
              im::protocol::v1::ERROR_CODE_UNSUPPORTED_MESSAGE_TYPE);

    response = handler.handle(im::protocol::v1::MESSAGE_TYPE_LOGIN_REQUEST, request);
    EXPECT_EQ(response.error_response().code(),
              im::protocol::v1::ERROR_CODE_MESSAGE_TYPE_MISMATCH);
}
