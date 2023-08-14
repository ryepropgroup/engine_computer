#set cmake version
cmake_minimum_required(VERSION 3.24)
project(engine_computer)
# set c standards
set(CMAKE_CXX_STANDARD 20)
# lsp support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# include ./lib
include_directories(lib)

# boost
include_directories(${Boost_INCLUDE_DIR})

if (APPLE)
    #    list(APPEND CMAKE_PREFIX_PATH ";/opt/homebrew/opt")
    set(LJM "/usr/local/lib/libLabJackM.dylib")
    set(LZ4 "/opt/homebrew/opt/lz4/lib/liblz4.dylib")
    set(ZSTD "/opt/homebrew/opt/pzstd/lib/libzstd.dylib")
elseif (UNIX)
    #    set(LABJACK_PATH "/usr/local/lib/libLabJackM.so")
    set(LJM "/usr/local/lib/libLabJackM.so")
    set(LZ4 "/usr/local/lib/liblz4.so")
    set(ZSTD "/usr/local/lib/libzstd.so")
endif ()
# Fetch system includes
include_directories("/usr/local/include/")

# Fetch External Library Declarations
include(FetchContent)
FetchContent_Declare(
        mcap
        GIT_REPOSITORY https://github.com/foxglove/mcap.git  # Replace with the actual repository URL
        GIT_TAG releases/cpp/v1.2.0  # Replace with the desired tag or commit hash
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
)
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)

# mcap
FetchContent_MakeAvailable(mcap)
add_library(mcap INTERFACE)
target_include_directories(mcap INTERFACE ${mcap_SOURCE_DIR}/cpp/mcap/include)

# json
FetchContent_MakeAvailable(json)
add_library(json INTERFACE)
target_include_directories(json INTERFACE ${json_SOURCE_DIR}/include)
add_executable(engine_computer main.cpp helpers.cpp)


# link all libs
target_link_libraries(engine_computer PRIVATE ${LJM} ${LZ4} ${ZSTD} mcap json)
