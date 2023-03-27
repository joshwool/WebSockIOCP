cmake_minimum_required(VERSION 3.24)
project(WebSockIOCP)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_TOOLCHAIN_FILE C:/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_PREFIX_PATH C:/vcpkg/installed/x64-windows)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)

add_definitions(-DSQLITE_ENABLE_COLUMN_METADATA)

add_executable(WebSockIOCP ${SOURCE_FILES} main.cpp)

add_subdirectory(json)

message(${SQLite3_LIBRARIES})

target_include_directories(WebSockIOCP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers ${SQLite3_INCLUDE_DIRS})
target_link_libraries(WebSockIOCP PUBLIC ws2_32.lib nlohmann_json::nlohmann_json OpenSSL::Crypto SQLite::SQLite3)

set(SQLITE_THREADSAFE 3)