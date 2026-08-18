#include "protocol/RequestHandler.h"

#include <gtest/gtest.h>

namespace
{
RequestHandler healthyHandler()
{
    return RequestHandler("im_chat", [](std::string& message) {
        message = "PostgreSQL connection is healthy";
        return true;
    });
}
}

TEST(RequestHandlerTests, HandlesPing)
{
    auto handler = healthyHandler();
    im::protocol::v1::Envelope request;
    request.set_protocol_version(1);
    request.set_request_id("ping-1");
    request.mutable_ping_request()->set_text("hello");

    const auto response = handler.handle(request);
    EXPECT_EQ(response.request_id(), "ping-1");
    ASSERT_TRUE(response.has_ping_response());
    EXPECT_EQ(response.ping_response().text(), "hello");
    EXPECT_GT(response.ping_response().server_time_unix_ms(), 0);
}

TEST(RequestHandlerTests, RejectsMissingRequestId)
{
    auto handler = healthyHandler();
    im::protocol::v1::Envelope request;
    request.set_protocol_version(1);
    request.mutable_ping_request()->set_text("hello");

    const auto response = handler.handle(request);
    ASSERT_TRUE(response.has_error_response());
    EXPECT_EQ(response.error_response().code(), 1003);
}

TEST(RequestHandlerTests, RejectsUnsupportedVersion)
{
    auto handler = healthyHandler();
    im::protocol::v1::Envelope request;
    request.set_protocol_version(2);
    request.set_request_id("version-1");
    request.mutable_ping_request();

    const auto response = handler.handle(request);
    ASSERT_TRUE(response.has_error_response());
    EXPECT_EQ(response.error_response().code(), 1002);
}

TEST(RequestHandlerTests, RejectsUnsupportedMessageType)
{
    auto handler = healthyHandler();
    im::protocol::v1::Envelope request;
    request.set_protocol_version(1);
    request.set_request_id("unsupported-1");

    const auto response = handler.handle(request);
    ASSERT_TRUE(response.has_error_response());
    EXPECT_EQ(response.error_response().code(), 1004);
}

TEST(RequestHandlerTests, ReportsDatabaseHealth)
{
    auto handler = healthyHandler();
    im::protocol::v1::Envelope request;
    request.set_protocol_version(1);
    request.set_request_id("db-1");
    request.mutable_database_health_request();

    const auto response = handler.handle(request);
    ASSERT_TRUE(response.has_database_health_response());
    EXPECT_TRUE(response.database_health_response().healthy());
    EXPECT_EQ(response.database_health_response().database_name(), "im_chat");
}

TEST(RequestHandlerTests, DoesNotExposeDatabaseFailureDetails)
{
    RequestHandler handler("im_chat", [](std::string& message) {
        message = "PostgreSQL is unavailable";
        return false;
    });
    im::protocol::v1::Envelope request;
    request.set_protocol_version(1);
    request.set_request_id("db-2");
    request.mutable_database_health_request();

    const auto response = handler.handle(request);
    ASSERT_TRUE(response.has_database_health_response());
    EXPECT_FALSE(response.database_health_response().healthy());
    EXPECT_EQ(response.database_health_response().message(),
              "PostgreSQL is unavailable");
}
