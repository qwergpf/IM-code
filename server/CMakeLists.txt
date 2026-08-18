find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

set(IM_PROTOCOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/protocol/im_protocol.proto)
protobuf_generate_cpp(IM_PROTO_SOURCES IM_PROTO_HEADERS ${IM_PROTOCOL_FILE})

add_library(im_protocol STATIC
    ${IM_PROTO_SOURCES}
    ${IM_PROTO_HEADERS}
    src/protocol/FrameCodec.cpp
    src/protocol/FrameCodec.h
    src/protocol/RequestHandler.cpp
    src/protocol/RequestHandler.h
)

target_include_directories(im_protocol
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(im_protocol
    PUBLIC
        protobuf::libprotobuf
)

add_library(im_server_core STATIC
    src/config/ServerConfig.cpp
    src/config/ServerConfig.h
    src/database/Database.cpp
    src/database/Database.h
    src/gateway/Session.cpp
    src/gateway/Session.h
    src/gateway/TcpServer.cpp
    src/gateway/TcpServer.h
)

target_include_directories(im_server_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(im_server_core
    PUBLIC
        im_protocol
        Boost::system
        Threads::Threads
        pqxx
)

add_executable(im_server src/main.cpp)
target_link_libraries(im_server PRIVATE im_server_core)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)

    add_executable(im_server_tests
        tests/unit/ConfigTests.cpp
        tests/unit/FrameCodecTests.cpp
        tests/unit/RequestHandlerTests.cpp
    )

    target_link_libraries(im_server_tests
        PRIVATE
            im_server_core
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(im_server_tests)
endif()
