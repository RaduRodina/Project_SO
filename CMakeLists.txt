cmake_minimum_required(VERSION 3.30)
project(untitled C)

set(CMAKE_C_STANDARD 99)

add_executable(untitled main.c
        treasure_hub.c
        monitor.c)
