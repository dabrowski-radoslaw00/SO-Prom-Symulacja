#include "common/config.h"
#include "common/shared_state.h"
#include "utils/passenger/random_passenger.h"
#include "utils/logger/colors.h"
#include "utils/logger/logger.h"
#include "ipc/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    pid_t my_pid = getpid();

    if (logger_init() == -1) {
        fprintf(stderr, "[PASSENGER-%d] Warning: Logger initialization failed\n", my_pid);
    }

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
        logger_cleanup();
        return EXIT_FAILURE;
    }

    int my_id = ipc_register_passenger(my_pid, attr.gender, attr.luggage_weight, attr.type);
    if (my_id == -1) {
        printf("%s[PASSENGER-%d]%s ERROR: No space available\n",
               C_ERROR, my_pid, COLOR_RESET);
        logger_cleanup();
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
        log_passenger_rejected(my_pid, attr.luggage_weight, MAX_LUGGAGE_WEIGHT);
        sleep(1);
        ipc_unregister_passenger(my_id);
        logger_cleanup();
        return EXIT_SUCCESS;
    } else {
        printf("%s[PASSENGER-%d] ✓ Luggage OK (%dkg <= %dkg)%s\n",
               C_SUCCESS, my_pid, attr.luggage_weight, MAX_LUGGAGE_WEIGHT, COLOR_RESET);
    }

    printf("%s[PASSENGER-%d]%s Waiting in hall...\n", C_INFO, my_pid, COLOR_RESET);
    sleep(2);

    printf("%s[PASSENGER-%d]%s Going through check-in...\n", C_INFO, my_pid, COLOR_RESET);
    sleep(1);


 printf("%s[PASSENGER-%d]%s Joining security control queue...%s\n",
           C_INFO, my_pid, COLOR_RESET,
           (attr.type == VIP) ? " [VIP PRIORITY]" : "");

    int queue_pos = ipc_join_security_queue(my_id);
    if (queue_pos < 0) {
        printf("%s[PASSENGER-%d]%s ERROR: Failed to join security queue\n",
               C_ERROR, my_pid, COLOR_RESET);
        ipc_unregister_passenger(my_id);
        logger_cleanup();
        return EXIT_FAILURE;
    }

    printf("%s[PASSENGER-%d]%s In security queue, position: %d\n",
           C_INFO, my_pid, COLOR_RESET, queue_pos);

    int station_id = -1;
    int assignment_attempts = 0;
    const int MAX_ASSIGNMENT_ATTEMPTS = 20;

    printf("%s[PASSENGER-%d]%s Waiting for security station assignment...\n",
           C_INFO, my_pid, COLOR_RESET);

    while (station_id < 0 && assignment_attempts < MAX_ASSIGNMENT_ATTEMPTS) {
        station_id = ipc_assign_to_security_station(my_id);

        if (station_id < 0) {
            int current_queue_pos = ipc_get_queue_position(my_id);
            if (current_queue_pos < 0) {
                ipc_lock();
                SystemState *state = ipc_get_state();
                if (state->passengers[my_id].security_station_id >= 0) {
                    station_id = state->passengers[my_id].security_station_id;
                    ipc_unlock();
                    break;
                }
                ipc_unlock();
            }

            usleep(500000);
            assignment_attempts++;
        }
    }

    if (station_id < 0) {
        printf("%s[PASSENGER-%d]%s ERROR: Failed to get security station assignment\n",
               C_ERROR, my_pid, COLOR_RESET);
        ipc_unregister_passenger(my_id);
        logger_cleanup();
        return EXIT_FAILURE;
    }

    printf("%s[PASSENGER-%d]%s Assigned to security station #%d\n",
           C_INFO, my_pid, COLOR_RESET, station_id);

    // Go through security check (simulate time)
    printf("%s[PASSENGER-%d]%s Going through security check...\n",
           C_INFO, my_pid, COLOR_RESET);

    #ifdef SECURITY_CHECK_TIME
        sleep(SECURITY_CHECK_TIME);
    #else
        sleep(2);
    #endif


    if (ipc_complete_security_check(my_id) != 0) {
        printf("%s[PASSENGER-%d]%s ERROR: Failed to complete security check\n",
               C_ERROR, my_pid, COLOR_RESET);
        ipc_unregister_passenger(my_id);
        logger_cleanup();
        return EXIT_FAILURE;
    }

    printf("%s[PASSENGER-%d] ✓ Security check passed%s\n",
           C_SUCCESS, my_pid, COLOR_RESET);

    printf("%s[PASSENGER-%d]%s In waiting room, ready to board...\n",
           C_INFO, my_pid, COLOR_RESET);

    printf("%s[PASSENGER-%d]%s Looking for available ferry...%s\n",
           C_INFO, my_pid, COLOR_RESET,
           (attr.type == VIP) ? " [VIP PRIORITY]" : "");

    int ferry_id = -1;
    int attempts = 0;
    const int MAX_ATTEMPTS = 10;

    const int SEARCH_DELAY = (attr.type == VIP) ? 500000 : 1000000;

    while (ferry_id == -1 && attempts < MAX_ATTEMPTS) {
        ferry_id = ipc_find_available_ferry(attr.luggage_weight);

        if (ferry_id == -1) {
            if (attr.type == VIP) {
                printf("%s[PASSENGER-%d] 👑 VIP waiting for ferry... (attempt %d/%d)%s\n",
                       C_VIP, my_pid, attempts + 1, MAX_ATTEMPTS, COLOR_RESET);
            } else {
                printf("%s[PASSENGER-%d]%s No ferry available, waiting... (attempt %d/%d)\n",
                       C_WARNING, my_pid, COLOR_RESET, attempts + 1, MAX_ATTEMPTS);
            }
            usleep(SEARCH_DELAY);
            attempts++;
        }
    }

    if (ferry_id == -1) {
        printf("%s[PASSENGER-%d]%s ERROR: No ferry found after %d attempts\n",
               C_ERROR, my_pid, COLOR_RESET, MAX_ATTEMPTS);
        ipc_unregister_passenger(my_id);
        logger_cleanup();
        return EXIT_FAILURE;
    }

    int ferry_max_luggage = ipc_get_ferry_max_luggage(ferry_id);

    printf("%s[PASSENGER-%d]%s Found Ferry #%d!%s\n",
           C_SUCCESS, my_pid, COLOR_RESET, ferry_id,
           (attr.type == VIP) ? " 👑" : "");

    printf("%s[PASSENGER-%d]%s Boarding Ferry #%d...%s\n",
           C_INFO, my_pid, COLOR_RESET, ferry_id,
           (attr.type == VIP) ? " [VIP]" : "");

    if (ipc_board_ferry(ferry_id, my_id) == -1) {
        printf("%s[PASSENGER-%d]%s ERROR: Failed to board Ferry #%d\n",
               C_ERROR, my_pid, COLOR_RESET, ferry_id);
        ipc_unregister_passenger(my_id);
        logger_cleanup();
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
    logger_cleanup();

    return EXIT_SUCCESS;
}