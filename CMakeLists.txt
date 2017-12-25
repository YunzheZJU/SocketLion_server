cmake_minimum_required(VERSION 3.9)
project(SocketLion_server)

set(CMAKE_CXX_STANDARD 11)

add_executable(SocketLion_server main.cpp)

# Linking
target_link_libraries(SocketLion_server ws2_32.lib)