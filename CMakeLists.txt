cmake_minimum_required(VERSION 3.21)

project(IMChat
    VERSION 0.1.0
    LANGUAGES CXX
)

include(CTest)

option(IM_BUILD_CLIENT "Build Qt desktop client" ON)
option(IM_BUILD_SERVER "Build Linux IM server" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(IM_BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(IM_BUILD_SERVER)
    add_subdirectory(server)
endif()
