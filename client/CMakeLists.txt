find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

qt_add_executable(im_client
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_link_libraries(im_client
    PRIVATE
        Qt6::Widgets
)

target_include_directories(im_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(im_client PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
