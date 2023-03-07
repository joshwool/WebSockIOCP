cmake_minimum_required(VERSION 3.24)
project(WebSockIOCP)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCE_FILES *.cpp)

foreach(SOURCE_PATH ${SOURCE_FILES})
    get_filename_component(EXECUTABLE_NAME ${SOURCE_PATH} NAME_WE)
    add_executable(WebSockIOCP ${EXECUTABLE_NAME} ${SOURCE_PATH})
endforeach(SOURCE_PATH ${SOURCE_FILES})

target_link_libraries(WebSockIOCP ws2_32.lib)
target_include_directories(WebSockIOCP PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/headers")