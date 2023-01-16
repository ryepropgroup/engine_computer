cmake_minimum_required(VERSION 3.23)
project(engine)

set(CMAKE_CXX_STANDARD 17)# see https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost REQUIRED)
add_executable(serial_test main.cpp)
target_link_libraries(jsoncpp)

include_directories(${Boost_INCLUDE_DIR})
include_directories(/home/gremlin/engine_computer/vcpkg/installed/x64-linux/include)
link_directories(/home/gremlin/engine_computer/vcpkg/installed/x64-linux/lib)

