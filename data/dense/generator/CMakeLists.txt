cmake_minimum_required(VERSION 3.22.1)
project(dense_generator C)

set(CMAKE_C_STANDARD 17)

add_executable(dense_generator
        main.c)

target_link_libraries(dense_generator m)
