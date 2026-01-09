#include "ipc/ipc.h"
#include "utils/logger/logger.h"
#include "utils/logger/colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <time.h>

#define CAPTAIN_WORK_TIME 12
#define SIGNAL2_AFTER 8

int main(void) {
    printf("%s[CAPTAIN]%s Port Captain starting...\n", COLOR_BRIGHT_YELLOW, COLOR_RESET);

    if (logger_init() == -1) {
        fprintf(stderr, "[CAPTAIN] Warning: Logger initialization failed\n");
    }


    if (ipc_attach() == -1) {
        fprintf(stderr, "[CAPTAIN] Failed to attach to IPC\n");
        logger_cleanup();
        return EXIT_FAILURE;
    }

    printf("%s[CAPTAIN]%s Connected to shared memory and semaphores\n",
           COLOR_BRIGHT_YELLOW, COLOR_RESET);

    log_message("Port captain started");

    printf("%s[CAPTAIN]%s Managing port operations...\n",
           COLOR_BRIGHT_YELLOW, COLOR_RESET);

    time_t start_time = time(NULL);
    bool signal2_sent = false;

    while (1) {
        sleep(1);

        time_t now = time(NULL);
        int elapsed = (int)difftime(now, start_time);

        printf("%s[CAPTAIN]%s Working... (%d/%d sec)\n",
               COLOR_BRIGHT_YELLOW, COLOR_RESET, elapsed, CAPTAIN_WORK_TIME);

        if (!signal2_sent && elapsed >= SIGNAL2_AFTER) {
            printf("%s[CAPTAIN] 📢 Sending SIGUSR2 - Stop accepting new passengers!%s\n",
                   COLOR_BRIGHT_RED, COLOR_RESET);

            ipc_set_boarding_allowed(false);
            ipc_send_signal_to_passengers(SIGUSR2);

            log_message("Captain sent SIGUSR2 - boarding stopped");
            signal2_sent = true;
        }

        if (elapsed >= CAPTAIN_WORK_TIME) {
            break;
        }
    }

    // Send SIGUSR1 to force remaining ferries to depart
    printf("%s[CAPTAIN] 📢 Sending SIGUSR1 - Force ferry departure!%s\n",
           COLOR_BRIGHT_RED, COLOR_RESET);

    ipc_set_force_departure(true);
    ipc_send_signal_to_ferries(SIGUSR1);

    log_message("Captain sent SIGUSR1 - force departure");

    printf("%s[CAPTAIN]%s Port Captain finished\n", COLOR_BRIGHT_YELLOW, COLOR_RESET);
    log_message("Port captain finished");

    logger_cleanup();
    
    return EXIT_SUCCESS;
}