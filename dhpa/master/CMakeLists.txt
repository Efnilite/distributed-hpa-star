cmake_minimum_required(VERSION 3.22.1)
project(master C)

set(CMAKE_C_STANDARD 17)

add_executable(master
        src/main.c
        src/preprocess.c
        ../common/graph_a.c
        src/master_a.c
        ../../common/parser.c
        ../../common/map.c
        ../../common/vec2.c
        ../../common/mheap.c
        ../../common/result.c
        ../../common/util.c
        ../../common/vbitset.c
        ../../common/graph.c
        ../../common/tcp.c)

target_link_libraries(master m)
