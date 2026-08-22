cmake_minimum_required(VERSION 3.26)

project(PPG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(PPG
    src/main.cpp
    src/Server.cpp
    src/Utils.cpp
    src/Parser.cpp
    src/ResponseBuilder.cpp
)

target_include_directories(PPG PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(PPG PRIVATE ws2_32)

target_compile_options(PPG PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        #-Werror
    )