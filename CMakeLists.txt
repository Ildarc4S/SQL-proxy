cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra")

project(SQLProxyServer)

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
add_executable(ProxyServer ${SOURCES})
