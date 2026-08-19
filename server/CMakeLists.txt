find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(MYSQLCLIENT QUIET IMPORTED_TARGET mysqlclient)
endif()

if(TARGET PkgConfig::MYSQLCLIENT)
    set(IM_MYSQL_TARGET PkgConfig::MYSQLCLIENT)
else()
    find_path(IM_MYSQL_INCLUDE_DIR NAMES mysql/mysql.h)
    find_library(IM_MYSQL_LIBRARY NAMES mysqlclient mariadb)
    if(NOT IM_MYSQL_INCLUDE_DIR OR NOT IM_MYSQL_LIBRARY)
        message(FATAL_ERROR "MySQL client development files were not found. Install default-libmysqlclient-dev.")
    endif()
    add_library(im_mysql_client UNKNOWN IMPORTED)
    set_target_properties(im_mysql_client PROPERTIES
        IMPORTED_LOCATION "${IM_MYSQL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${IM_MYSQL_INCLUDE_DIR}"
    )
    set(IM_MYSQL_TARGET im_mysql_client)
endif()

set(IM_PROTOCOL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/protocol/im_protocol.proto")
protobuf_generate_cpp(IM_PROTO_SOURCES IM_PROTO_HEADERS "${IM_PROTOCOL_FILE}")

set(IM_PYTHON_PROTO "${CMAKE_CURRENT_BINARY_DIR}/im_protocol_pb2.py")
add_custom_command(
    OUTPUT "${IM_PYTHON_PROTO}"
    COMMAND "${Protobuf_PROTOC_EXECUTABLE}"
        --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/protocol"
        --python_out="${CMAKE_CURRENT_BINARY_DIR}"
        "${IM_PROTOCOL_FILE}"
    DEPENDS "${IM_PROTOCOL_FILE}"
    COMMENT "Generating Python Protobuf module"
)
add_custom_target(im_python_protocol DEPENDS "${IM_PYTHON_PROTO}")

add_library(im_server_core STATIC
    ${IM_PROTO_SOURCES}
    ${IM_PROTO_HEADERS}
    src/config/ConfigManager.cpp
    src/config/ConfigManager.h
    src/database/DatabaseManager.cpp
    src/database/DatabaseManager.h
    src/logging/Logger.cpp
    src/logging/Logger.h
    src/network/ClientConnection.cpp
    src/network/ClientConnection.h
    src/network/NetworkServer.cpp
    src/network/NetworkServer.h
    src/protocol/PacketCodec.cpp
    src/protocol/PacketCodec.h
    src/protocol/ProtocolHandler.cpp
    src/protocol/ProtocolHandler.h
    src/server/ServerApplication.cpp
    src/server/ServerApplication.h
)

target_include_directories(im_server_core
    PUBLIC
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(im_server_core
    PUBLIC
        Boost::system
        Threads::Threads
        protobuf::libprotobuf
        "${IM_MYSQL_TARGET}"
)

target_compile_features(im_server_core PUBLIC cxx_std_17)

add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE im_server_core)
set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config/server.conf"
    "${CMAKE_BINARY_DIR}/server.conf"
    COPYONLY
)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_executable(im_server_tests
        tests/unit/ConfigManagerTests.cpp
        tests/unit/PacketCodecTests.cpp
        tests/unit/ProtocolHandlerTests.cpp
    )
    target_link_libraries(im_server_tests PRIVATE im_server_core GTest::gtest_main)
    target_include_directories(im_server_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
    gtest_discover_tests(im_server_tests)
    add_dependencies(im_server_tests im_python_protocol)
endif()
