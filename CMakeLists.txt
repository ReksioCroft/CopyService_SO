cmake_minimum_required(VERSION 3.17)
project(CopyService_SO C)

set(CMAKE_C_STANDARD 11)

add_executable(CopyService_Client myClient.c channel.c channel.h )
add_executable(CopyService_Daemon myDaemon.c channel.c channel.h)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
