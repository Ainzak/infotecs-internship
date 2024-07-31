cmake_minimum_required(VERSION 3.26)
project(infotecs)

set(CMAKE_CXX_STANDARD 17)


add_executable(server server/main.cpp)

add_executable(client client/main.cpp)