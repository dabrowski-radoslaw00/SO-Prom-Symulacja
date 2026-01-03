cmake_minimum_required(VERSION 3.18.4)
project(so_ferry_simulation C)

set(CMAKE_C_STANDARD 11)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

add_executable(main
        ${SRC_DIR}/main.c
)
