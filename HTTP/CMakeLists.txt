cmake_minimum_required(VERSION 3.26)

project(PPG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(PPG
    src/main.cpp
    src/Server.cpp
    src/Utils.cpp
    src/Parser.cpp
    src/ResponseBuilder.cpp
    src/Logger.cpp
)

target_include_directories(PPG PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(PPG PRIVATE ws2_32)

target_compile_options(PPG PRIVATE
	-Wall
	-Wextra
	-Wpedantic
	-Wconversion
	-Wsign-conversion
	-Wshadow
	-Wcast-align
	-Wcast-qual
	-Wformat=2
	-Wnull-dereference
	-Wdouble-promotion
	-Wimplicit-fallthrough
	-Woverloaded-virtual
	-Wnon-virtual-dtor
	-Wold-style-cast
	-Werror=vla
	-Wundef
	-Wctor-dtor-privacy
	-Wextra-semi
	-Wsign-promo
	-Wzero-as-null-pointer-constant
)