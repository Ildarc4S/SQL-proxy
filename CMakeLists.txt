cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(SQLProxyServer)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(ProxyServer
               ${CMAKE_SOURCE_DIR}/src/ProxyServer.cpp
               ${CMAKE_SOURCE_DIR}/src/main.cpp
               )
