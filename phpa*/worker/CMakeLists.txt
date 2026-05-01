cmake_minimum_required(VERSION 3.22.1)
project(wphpa C)

set(CMAKE_C_STANDARD 17)

add_executable(wphpa
        src/main.c
        src/worker_a.c
        ../../common/parser.c
        ../../common/map.c
        ../../common/vec2.c
        ../../common/mheap.c
        ../../common/result.c
        ../../common/vbitset.c
        ../../common/graph.c
        ../../common/tcp.c)

target_link_libraries(wphpa m)
