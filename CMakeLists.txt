#set cmake version
cmake_minimum_required(VERSION 3.27.2)
project(engine_computer)
# set c standards
set(CMAKE_CXX_STANDARD 20)
message("C++ VERSION -> 20")
# lsp support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# boost
find_package(Boost REQUIRED)
Message("BOOST FOUND")

# protobuf
find_package(Protobuf REQUIRED)
IF (NOT EXISTS "${Protobuf_PROTOC_EXECUTABLE}")
    MESSAGE(FATAL_ERROR "PROTOC NOT FOUND!")
ENDIF ()
Message("PROTOC FOUND")
Message(${PROTOBUF_LIBRARY})
add_subdirectory(proto)

# include directories
include_directories(lib)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})
# Fetch system includes
include_directories("/usr/local/include/")

if (APPLE)
    message("SETTING Apple Library Paths")
    #    list(APPEND CMAKE_PREFIX_PATH ";/opt/homebrew/opt")
    set(LJM "/usr/local/lib/libLabJackM.dylib")
    set(PROTOBUF "/opt/homebrew/lib/libprotobuf.dylib")
elseif (UNIX)
    message("SETTING Unix/Linux Library Paths!")
    #    set(LABJACK_PATH "/usr/local/lib/libLabJackM.so")
    set(LJM "/usr/local/lib/libLabJackM.so")
endif ()
# Fetch External Library Declarations
include(FetchContent)
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
message("CLONED json")

message("LINKING TO EXECUTABLE")
add_executable(engine_computer main.cpp helpers.cpp)
# json
FetchContent_MakeAvailable(json)
add_library(json INTERFACE)
target_include_directories(json INTERFACE ${json_SOURCE_DIR}/include)


# link all libs
target_link_libraries(engine_computer PRIVATE ${LJM} ${PROTOBUF_LIBRARIES} ${BOOST_Libraries} json)