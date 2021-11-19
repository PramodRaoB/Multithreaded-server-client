cmake_minimum_required(VERSION 3.21)
project(Multithreaded_server_client)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-lpthread")

include_directories(.)

add_executable(Multithreaded_server_client
        server.cpp
        server.h)
#        server.h client.cpp client.h globals.h)
