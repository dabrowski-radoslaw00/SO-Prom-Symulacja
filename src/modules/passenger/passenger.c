#include "common/config.h"
#include "common/shared_state.h"
#include "utils/passenger/random_passenger.h"
#include "utils/logger/colors.h"
#include "ipc/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    pid_t my_pid = getpid();

    PassengerAttributes attr = random_passenger_attributes();

    const char *gender_str = (attr.gender == MALE) ? "MALE" : "FEMALE";
    const char *gender_color = (attr.gender == MALE) ? C_MALE : C_FEMALE;
    const char *type_str = (attr.type == VIP) ? "VIP" : "REGULAR";
    const char *type_color = (attr.type == VIP) ? C_VIP : COLOR_WHITE;

    printf("%s[PASSENGER-%d]%s Starting... [%s%s%s, %s%dkg%s, %s%s%s]\n",
           C_INFO, my_pid, COLOR_RESET,
           gender_color, gender_str, COLOR_RESET,
           COLOR_YELLOW, attr.luggage_weight, COLOR_RESET,
           type_color, type_str, COLOR_RESET);

    if (ipc_attach() == -1) {
        fprintf(stderr, "[PASSENGER-%d] Failed to attach to IPC\n", my_pid);
        return EXIT_FAILURE;
    }

    int my_id = ipc_register_passenger(my_pid, attr.gender, attr.luggage_weight, attr.type);
    if (my_id == -1) {
        printf("%s[PASSENGER-%d]%s ERROR: No space available\n",
               C_ERROR, my_pid, COLOR_RESET);
        return EXIT_FAILURE;
    }

    printf("%s[PASSENGER-%d]%s Registered with ID: %s%d%s\n",
           C_INFO, my_pid, COLOR_RESET,
           STYLE_BOLD, my_id, COLOR_RESET);

    if (attr.luggage_weight > MAX_LUGGAGE_WEIGHT) {
        printf("%s[PASSENGER-%d] ❌ Luggage too heavy (%dkg > %dkg) - REJECTED%s\n",
               C_ERROR, my_pid, attr.luggage_weight, MAX_LUGGAGE_WEIGHT, COLOR_RESET);
        printf("%s[PASSENGER-%d]%s Returning to check-in hall...\n",
               C_WARNING, my_pid, COLOR_RESET);

        sleep(1);
        ipc_unregister_passenger(my_id);
        return EXIT_SUCCESS;
    } else {
        printf("%s[PASSENGER-%d] ✓ Luggage OK (%dkg <= %dkg)%s\n",
               C_SUCCESS, my_pid, attr.luggage_weight, MAX_LUGGAGE_WEIGHT, COLOR_RESET);
    }

    printf("%s[PASSENGER-%d]%s Waiting in hall...\n", C_INFO, my_pid, COLOR_RESET);
    sleep(2);

    printf("%s[PASSENGER-%d]%s Going through check-in...\n", C_INFO, my_pid, COLOR_RESET);
    sleep(1);

    printf("%s[PASSENGER-%d]%s Looking for available ferry...\n", C_INFO, my_pid, COLOR_RESET);

    int ferry_id = -1;
    int attempts = 0;
    const int MAX_ATTEMPTS = 10;

    while (ferry_id == -1 && attempts < MAX_ATTEMPTS) {
        ferry_id = ipc_find_available_ferry(attr.luggage_weight);

        if (ferry_id == -1) {
            printf("%s[PASSENGER-%d]%s No ferry available, waiting... (attempt %d/%d)\n",
                   C_WARNING, my_pid, COLOR_RESET, attempts + 1, MAX_ATTEMPTS);
            sleep(1);
            attempts++;
        }
    }

    if (ferry_id == -1) {
        printf("%s[PASSENGER-%d]%s ERROR: No ferry found after %d attempts\n",
               C_ERROR, my_pid, COLOR_RESET, MAX_ATTEMPTS);
        ipc_unregister_passenger(my_id);
        return EXIT_FAILURE;
    }

    printf("%s[PASSENGER-%d]%s Found Ferry #%d!\n",
           C_SUCCESS, my_pid, COLOR_RESET, ferry_id);

    printf("%s[PASSENGER-%d]%s Boarding Ferry #%d...\n",
           C_INFO, my_pid, COLOR_RESET, ferry_id);

    if (ipc_board_ferry(ferry_id, my_id) == -1) {
        printf("%s[PASSENGER-%d]%s ERROR: Failed to board Ferry #%d\n",
               C_ERROR, my_pid, COLOR_RESET, ferry_id);
        ipc_unregister_passenger(my_id);
        return EXIT_FAILURE;
    }

    printf("%s[PASSENGER-%d] ⛴️  On board Ferry #%d!%s\n",
           C_SUCCESS, my_pid, ferry_id, COLOR_RESET);

    printf("%s[PASSENGER-%d]%s Waiting for departure...\n",
           STYLE_DIM, my_pid, COLOR_RESET);

    sleep(FERRY_TRIP_TIME + 2);

    printf("%s[PASSENGER-%d] ✓ Journey completed%s\n", C_SUCCESS, my_pid, COLOR_RESET);

    ipc_unregister_passenger(my_id);
    printf("%s[PASSENGER-%d]%s Unregistered\n", STYLE_DIM, my_pid, COLOR_RESET);

    return EXIT_SUCCESS;
}