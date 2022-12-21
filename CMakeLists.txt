cmake_minimum_required(VERSION 3.23)
project(serial_test)

set(CMAKE_CXX_STANDARD 17)# see https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

add_executable(serial_test main.cpp)
