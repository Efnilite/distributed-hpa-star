cmake_minimum_required(VERSION 3.22.1)
project(astar)

set(CMAKE_C_STANDARD 17)

add_executable(astar src/main.c ../common/parser.c ../common/map.c
        ../common/vec.c
        ../common/vec.h)
