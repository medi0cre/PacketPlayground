cmake_minimum_required(VERSION 3.26)

project(PacketPlayground LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(PacketPlayground
    main.cpp
    Server.cpp
    Utils.cpp
)

target_include_directories(PacketPlayground PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PacketPlayground PRIVATE ws2_32)