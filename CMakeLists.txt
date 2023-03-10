cmake_minimum_required(VERSION 3.24)
project(WebSockIOCP)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

add_executable(WebSockIOCP ${SOURCE_FILES} main.cpp)

target_link_libraries(WebSockIOCP ws2_32.lib)
target_include_directories(WebSockIOCP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)