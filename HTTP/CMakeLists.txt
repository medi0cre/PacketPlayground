cmake_minimum_required(VERSION 3.26)

project(PPG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(PPG
    main.cpp
    Server.cpp
    Utils.cpp
)

target_include_directories(PPG PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PPG PRIVATE ws2_32)