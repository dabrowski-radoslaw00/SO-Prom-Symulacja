cmake_minimum_required(VERSION 3.18.4)
project(so_ferry_simulation C)

set(CMAKE_C_STANDARD 11)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

set(COMMON_SOURCES
        ${SRC_DIR}/ipc/ipc.c
        ${SRC_DIR}/utils/logger.c
)

add_executable(captain
        ${SRC_DIR}/modules/captain/captain.c
        ${COMMON_SOURCES}
)
target_include_directories(captain PRIVATE ${SRC_DIR})

add_executable(test_logger
        ${SRC_DIR}/test_logger.c
        ${COMMON_SOURCES}
)
target_include_directories(test_logger PRIVATE ${SRC_DIR})

add_executable(main
        ${SRC_DIR}/main.c
        ${COMMON_SOURCES}
)
target_include_directories(main PRIVATE ${SRC_DIR})