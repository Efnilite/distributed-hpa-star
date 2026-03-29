cmake_minimum_required(VERSION 3.22.1)
project(a C)

set(CMAKE_C_STANDARD 17)

add_executable(a
        src/a.c
        ../common/main.c
        ../common/parser.c
        ../common/graph.c
        ../common/map.c
        ../common/vec2.c
        ../common/mheap.c
        ../common/result.c
        ../common/vbitset.c)

target_link_libraries(a m)
