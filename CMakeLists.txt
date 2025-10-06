cmake_minimum_required(VERSION 3.10)

project(parser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(parser PRIVATE
    src/main.cpp
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/core/parser)