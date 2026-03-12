cmake_minimum_required(VERSION 3.22.1)
project(astar C)

set(CMAKE_C_STANDARD 17)

add_executable(astar src/main.c
        ../common/parser.c
        ../common/map.c
        ../common/vec2.c
        ../common/mheap.c
        ../common/result.c
        src/a.c
        ../common/vbitset.c)

target_link_libraries(astar m)
