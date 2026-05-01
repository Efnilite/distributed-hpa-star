cmake_minimum_required(VERSION 3.22.1)
project(hpa C)

set(CMAKE_C_STANDARD 17)

add_executable(hpa
        src/main.c
        src/hpa.c
        src/graph_a.c
        src/cluster_a.c
        ../../common/main.c
        ../../common/parser.c
        ../../common/map.c
        ../../common/vec2.c
        ../../common/mheap.c
        ../../common/result.c
        ../../common/vbitset.c
        ../../common/graph.c)

target_link_libraries(hpa m)
