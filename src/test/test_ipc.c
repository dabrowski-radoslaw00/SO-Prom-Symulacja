#include "ipc/ipc.h"
#include "common/config.h"
#include "utils/logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TEST_PASSED 0
#define TEST_FAILED 1

static int tests_passed = 0;
static int tests_failed = 0;

void test_result(const char *test_name, int result) {
    if (result == TEST_PASSED) {
        printf("✓ %s\n", test_name);
        tests_passed++;
    } else {
        printf("✗ %s\n", test_name);
        tests_failed++;
    }
}

int test_ipc_init(void) {
    printf("\n=== Test 1: IPC Initialization ===\n");

    if (ipc_init() == -1) {
        printf("ERROR: ipc_init() failed\n");
        return TEST_FAILED;
    }

    SystemState *state = ipc_get_state();
    if (state == NULL) {
        printf("ERROR: ipc_get_state() returned NULL\n");
        ipc_cleanup();
        return TEST_FAILED;
    }

    if (!state->system_running) {
        printf("ERROR: system_running should be true\n");
        ipc_cleanup();
        return TEST_FAILED;
    }

    if (state->total_passengers != 0) {
        printf("ERROR: total_passengers should be 0 initially\n");
        ipc_cleanup();
        return TEST_FAILED;
    }

    if (state->total_ferries != 0) {
        printf("ERROR: total_ferries should be 0 initially\n");
        ipc_cleanup();
        return TEST_FAILED;
    }

    printf("  - Shared memory created successfully\n");
    printf("  - System state initialized correctly\n");

    return TEST_PASSED;
}

int test_passenger_registration(void) {
    printf("\n=== Test 2: Passenger Registration ===\n");

    pid_t test_pid = getpid();

    // Test 2.1: Register first passenger
    int id1 = ipc_register_passenger(test_pid, MALE, 15, REGULAR);
    if (id1 < 0 || id1 >= MAX_PASSENGERS) {
        printf("ERROR: Failed to register passenger 1\n");
        return TEST_FAILED;
    }
    printf("  - Passenger 1 registered with ID: %d\n", id1);

    SystemState *state = ipc_get_state();
    if (state->passengers[id1].active != true) {
        printf("ERROR: Passenger 1 not marked as active\n");
        return TEST_FAILED;
    }

    if (state->passengers[id1].gender != MALE) {
        printf("ERROR: Passenger 1 gender incorrect\n");
        return TEST_FAILED;
    }

    if (state->passengers[id1].luggage_weight != 15) {
        printf("ERROR: Passenger 1 luggage weight incorrect\n");
        return TEST_FAILED;
    }

    if (state->total_passengers != 1) {
        printf("ERROR: total_passengers should be 1, got %d\n", state->total_passengers);
        return TEST_FAILED;
    }

    // Test 2.2: Register second passenger
    int id2 = ipc_register_passenger(test_pid, FEMALE, 20, VIP);
    if (id2 < 0 || id2 >= MAX_PASSENGERS || id2 == id1) {
        printf("ERROR: Failed to register passenger 2\n");
        return TEST_FAILED;
    }
    printf("  - Passenger 2 registered with ID: %d\n", id2);

    if (state->total_passengers != 2) {
        printf("ERROR: total_passengers should be 2, got %d\n", state->total_passengers);
        return TEST_FAILED;
    }

    if (state->passengers[id2].type != VIP) {
        printf("ERROR: Passenger 2 should be VIP\n");
        return TEST_FAILED;
    }

    // Test 2.3: Unregister passenger
    ipc_unregister_passenger(id1);
    if (state->passengers[id1].active != false) {
        printf("ERROR: Passenger 1 should be inactive after unregister\n");
        return TEST_FAILED;
    }

    if (state->total_passengers != 1) {
        printf("ERROR: total_passengers should be 1 after unregister, got %d\n",
               state->total_passengers);
        return TEST_FAILED;
    }
    printf("  - Passenger unregistration works correctly\n");

    return TEST_PASSED;
}

int test_ferry_registration(void) {
    printf("\n=== Test 3: Ferry Registration ===\n");

    pid_t test_pid = getpid();

    int ferry_id1 = ipc_register_ferry(test_pid, 25);
    if (ferry_id1 < 0 || ferry_id1 >= MAX_FERRIES) {
        printf("ERROR: Failed to register ferry 1\n");
        return TEST_FAILED;
    }
    printf("  - Ferry 1 registered with ID: %d, max_luggage: 25kg\n", ferry_id1);

    SystemState *state = ipc_get_state();
    if (state->ferries[ferry_id1].active != true) {
        printf("ERROR: Ferry 1 not marked as active\n");
        return TEST_FAILED;
    }

    if (state->ferries[ferry_id1].max_luggage_weight != 25) {
        printf("ERROR: Ferry 1 max_luggage_weight incorrect\n");
        return TEST_FAILED;
    }

    if (state->ferries[ferry_id1].num_passengers != 0) {
        printf("ERROR: Ferry 1 should have 0 passengers initially\n");
        return TEST_FAILED;
    }

    if (state->total_ferries != 1) {
        printf("ERROR: total_ferries should be 1, got %d\n", state->total_ferries);
        return TEST_FAILED;
    }

    int ferry_id2 = ipc_register_ferry(test_pid, 30);
    if (ferry_id2 < 0 || ferry_id2 >= MAX_FERRIES || ferry_id2 == ferry_id1) {
        printf("ERROR: Failed to register ferry 2\n");
        return TEST_FAILED;
    }
    printf("  - Ferry 2 registered with ID: %d, max_luggage: 30kg\n", ferry_id2);

    if (state->ferries[ferry_id2].max_luggage_weight != 30) {
        printf("ERROR: Ferry 2 max_luggage_weight incorrect\n");
        return TEST_FAILED;
    }

    if (state->total_ferries != 2) {
        printf("ERROR: total_ferries should be 2, got %d\n", state->total_ferries);
        return TEST_FAILED;
    }

    return TEST_PASSED;
}

int test_find_available_ferry(void) {
    printf("\n=== Test 4: Find Available Ferry ===\n");

    SystemState *state = ipc_get_state();

    // Set BOTH ferries to BOARDING status
    ipc_lock();
    if (state->ferries[0].active) {
        state->ferries[0].status = FERRY_BOARDING;
        printf("  - Set Ferry 0 to BOARDING status\n");
    }
    if (state->ferries[1].active) {
        state->ferries[1].status = FERRY_BOARDING;
        printf("  - Set Ferry 1 to BOARDING status\n");
    }
    ipc_unlock();

    // Test 4.1: Find ferry for luggage within limit
    int found = ipc_find_available_ferry(20);  // 20kg <= 25kg (ferry 0 limit)
    if (found != 0) {
        printf("ERROR: Should find ferry 0 for 20kg luggage, got %d\n", found);
        return TEST_FAILED;
    }
    printf("  - Found ferry for 20kg luggage (within limit)\n");

    found = ipc_find_available_ferry(28);  // 28kg > 25kg, but <= 30kg (ferry 1)
    if (found != 1) {
        printf("ERROR: Should find ferry 1 for 28kg luggage, got %d\n", found);
        return TEST_FAILED;
    }
    printf("  - Found ferry for 28kg luggage (ferry 1)\n");

    found = ipc_find_available_ferry(35);  // 35kg > both limits
    if (found != -1) {
        printf("ERROR: Should not find ferry for 35kg luggage, got %d\n", found);
        return TEST_FAILED;
    }
    printf("  - Correctly rejected 35kg luggage (exceeds all limits)\n");

    return TEST_PASSED;
}

int test_board_ferry(void) {
    printf("\n=== Test 5: Board Ferry ===\n");

    pid_t test_pid = getpid();
    int passenger_id = ipc_register_passenger(test_pid, MALE, 20, REGULAR);
    if (passenger_id < 0) {
        printf("ERROR: Failed to register passenger for boarding test\n");
        return TEST_FAILED;
    }
    printf("  - Registered passenger ID: %d\n", passenger_id);

    SystemState *state = ipc_get_state();
    int ferry_id = 0;

    ipc_lock();
    state->ferries[ferry_id].status = FERRY_BOARDING;
    state->ferries[ferry_id].num_passengers = 0;
    ipc_unlock();

    if (ipc_board_ferry(ferry_id, passenger_id) != 0) {
        printf("ERROR: Failed to board ferry\n");
        return TEST_FAILED;
    }
    printf("  - Passenger boarded ferry successfully\n");

    ipc_lock();
    if (state->ferries[ferry_id].num_passengers != 1) {
        printf("ERROR: Ferry should have 1 passenger, got %d\n",
               state->ferries[ferry_id].num_passengers);
        ipc_unlock();
        return TEST_FAILED;
    }

    if (state->passengers[passenger_id].status != STATUS_ON_FERRY) {
        printf("ERROR: Passenger status should be ON_FERRY\n");
        ipc_unlock();
        return TEST_FAILED;
    }

    if (state->passengers[passenger_id].ferry_id != ferry_id) {
        printf("ERROR: Passenger ferry_id should be %d, got %d\n",
               ferry_id, state->passengers[passenger_id].ferry_id);
        ipc_unlock();
        return TEST_FAILED;
    }
    ipc_unlock();

    printf("  - Passenger status updated correctly\n");

    if (ipc_is_ferry_full(ferry_id)) {
        printf("ERROR: Ferry should not be full with 1 passenger\n");
        return TEST_FAILED;
    }

    ipc_lock();
    state->ferries[ferry_id].num_passengers = FERRY_CAPACITY;
    ipc_unlock();

    if (!ipc_is_ferry_full(ferry_id)) {
        printf("ERROR: Ferry should be full with %d passengers\n", FERRY_CAPACITY);
        return TEST_FAILED;
    }
    printf("  - Ferry full detection works correctly\n");

    return TEST_PASSED;
}

int main(void) {
    printf("========================================\n");
    printf("  Basic IPC Verification Test Suite\n");
    printf("========================================\n");

    if (logger_init() == -1) {
        fprintf(stderr, "Warning: Logger initialization failed, continuing anyway\n");
    }

    test_result("IPC Initialization", test_ipc_init());
    test_result("Passenger Registration", test_passenger_registration());
    test_result("Ferry Registration", test_ferry_registration());
    test_result("Find Available Ferry", test_find_available_ferry());
    test_result("Board Ferry", test_board_ferry());


    ipc_cleanup();
    logger_cleanup();

    printf("\n========================================\n");
    printf("  Test Summary\n");
    printf("========================================\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    if (tests_failed == 0) {
        printf("\n✓ All tests passed!\n");
        return EXIT_SUCCESS;
    }

    printf("\n✗ Some tests failed\n");
    return EXIT_FAILURE;
}