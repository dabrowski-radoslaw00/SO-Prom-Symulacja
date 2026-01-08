cmake_minimum_required(VERSION 3.18.4)
project(so_ferry_simulation C)

set(CMAKE_C_STANDARD 11)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

set(TEST_DIR ${SRC_DIR}/test)
set(SRC_UTILS ${SRC_DIR}/utils)

set(COMMON_SOURCES
        ${SRC_DIR}/ipc/ipc.c
        ${SRC_UTILS}/logger/logger.c
        ${SRC_UTILS}/passenger/random_passenger.c
)

add_executable(main
        ${SRC_DIR}/main.c
        ${COMMON_SOURCES}
)

target_include_directories(main PRIVATE ${SRC_DIR})

add_executable(captain
        ${SRC_DIR}/modules/captain/captain.c
        ${COMMON_SOURCES}
)
target_include_directories(captain PRIVATE ${SRC_DIR})

add_executable(passenger
        ${SRC_DIR}/modules/passenger/passenger.c
        ${COMMON_SOURCES}
)
target_include_directories(passenger PRIVATE ${SRC_DIR})

add_executable(ferry
        ${SRC_DIR}/modules/ferry/ferry.c
        ${COMMON_SOURCES}
)
target_include_directories(ferry PRIVATE ${SRC_DIR})


add_executable(test_logger
        ${TEST_DIR}/test_logger.c
        ${COMMON_SOURCES}
)

add_executable(test_ipc
        ${TEST_DIR}/test_ipc.c
        ${COMMON_SOURCES}
)

target_include_directories(test_ipc PRIVATE ${SRC_DIR})

add_executable(test_security
        ${TEST_DIR}/test_security.c
        ${COMMON_SOURCES}
)

target_include_directories(test_security PRIVATE ${SRC_DIR})

target_include_directories(test_logger PRIVATE ${SRC_DIR})
