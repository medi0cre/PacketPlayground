cmake_minimum_required(VERSION 3.26)

project(MyServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(MyServer
    main.cpp
    Server.cpp
)

target_include_directories(MyServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MyServer PRIVATE ws2_32)