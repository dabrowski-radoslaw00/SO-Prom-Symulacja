#include "utils/logger.h"
#include "ipc/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_PASSENGERS 5

int main(void) {
    printf("=== Ferry System - Main Launcher ===\n\n");

    printf("Initializing logger...\n");
    if (logger_init() == -1) {
        fprintf(stderr, "Failed to initialize logger\n");
        return EXIT_FAILURE;
    }
    log_message("=== SYSTEM STARTUP ===");
    log_message("Logger initialized");

    printf("Initializing IPC...\n");
    if (ipc_init() == -1) {
        fprintf(stderr, "Failed to initialize IPC\n");
        log_message("ERROR: Failed to initialize IPC");
        logger_cleanup();
        return EXIT_FAILURE;
    }
    log_message("IPC initialized (shared memory + semaphores)");

    printf("Starting port captain...\n");
    log_message("Forking port captain process");

    pid_t captain_pid = fork();

    if (captain_pid == -1) {
        perror("fork captain");
        log_message("ERROR: Failed to fork captain process");
        ipc_cleanup();
        logger_cleanup();
        return EXIT_FAILURE;
    }

    if (captain_pid == 0) {
        execl("./captain", "captain", NULL);
        perror("execl captain");
        exit(EXIT_FAILURE);
    }

    log_message("Port captain process started");
    printf("Port captain running (PID: %d)\n\n", captain_pid);

    sleep(1);

    printf("Creating %d passengers...\n", NUM_PASSENGERS);
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Creating %d passenger processes", NUM_PASSENGERS);
    log_message(log_msg);

    pid_t passenger_pids[NUM_PASSENGERS];

    for (int i = 0; i < NUM_PASSENGERS; i++) {
        passenger_pids[i] = fork();

        if (passenger_pids[i] == -1) {
            perror("fork passenger");
            snprintf(log_msg, sizeof(log_msg), "ERROR: Failed to fork passenger %d", i);
            log_message(log_msg);
            continue;
        }

        if (passenger_pids[i] == 0) {
            execl("./passenger", "passenger", NULL);
            perror("execl passenger");
            exit(EXIT_FAILURE);
        }

        printf("  Passenger %d started (PID: %d)\n", i+1, passenger_pids[i]);

        usleep(100000);
    }

    log_message("All passenger processes created");
    printf("\nAll processes running...\n\n");

    printf("Waiting for passengers to finish...\n");
    for (int i = 0; i < NUM_PASSENGERS; i++) {
        if (passenger_pids[i] > 0) {
            int status;
            waitpid(passenger_pids[i], &status, 0);
            if (WIFEXITED(status)) {
                printf("  Passenger (PID: %d) finished\n", passenger_pids[i]);
            }
        }
    }
    log_message("All passengers finished");

    printf("\nWaiting for captain to finish...\n");
    int status;
    waitpid(captain_pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("Port captain finished with status: %d\n", WEXITSTATUS(status));
        log_message("Port captain finished successfully");
    } else {
        log_message("WARNING: Port captain terminated abnormally");
    }

    printf("\nCleaning up...\n");
    log_message("Starting system cleanup");

    if (ipc_cleanup() == -1) {
        fprintf(stderr, "Warning: IPC cleanup had errors\n");
        log_message("WARNING: IPC cleanup had errors");
    } else {
        log_message("IPC cleaned up successfully");
    }

    log_message("=== SYSTEM SHUTDOWN ===");
    logger_cleanup();

    printf("\n=== System finished ===\n");

    return EXIT_SUCCESS;
}