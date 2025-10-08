cmake_minimum_required(VERSION 3.10)

project(parser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG "-g")

add_executable(parser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/core/parser)

target_link_libraries(parser PRIVATE
    core_parser
)