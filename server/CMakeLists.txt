find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# Newer libpqxx releases provide a CMake package, while Ubuntu 22.04's
# libpqxx-dev 6.4 package only provides pkg-config metadata. Support both.
find_package(libpqxx CONFIG QUIET)
if(TARGET libpqxx::pqxx)
    set(IM_PQXX_TARGET libpqxx::pqxx)
elseif(TARGET pqxx)
    set(IM_PQXX_TARGET pqxx)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)
    set(IM_PQXX_TARGET PkgConfig::PQXX)
endif()

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
        ${IM_PQXX_TARGET}
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
