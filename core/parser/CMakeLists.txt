cmake_minimum_required(VERSION 3.10)

project(core_parser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG "-g")

add_library(core_parser STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp
)

target_include_directories(core_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)