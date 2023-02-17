cmake_minimum_required(VERSION 3.24)
project(engine_computer)

set(CMAKE_CXX_STANDARD 20)

add_executable(engine_computer main.cpp)
include_directories(lib)
# see https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
