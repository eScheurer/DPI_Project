cmake_minimum_required(VERSION 3.10)
project(DPI-Project C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(OS_Chat
        main.c
        crdt.c
)