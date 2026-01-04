#include "utils/logger.h"
#include "ipc/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    printf("=== Ferry System - Main Launcher ===\n\n");

    // 1. Inicjalizacja loggera
    printf("Initializing logger...\n");
    if (logger_init() == -1) {
        fprintf(stderr, "Failed to initialize logger\n");
        return EXIT_FAILURE;
    }
    log_message("=== SYSTEM STARTUP ===");
    log_message("Logger initialized");

    // 2. Inicjalizacja IPC
    printf("Initializing IPC...\n");
    if (ipc_init() == -1) {
        fprintf(stderr, "Failed to initialize IPC\n");
        log_message("ERROR: Failed to initialize IPC");
        logger_cleanup();
        return EXIT_FAILURE;
    }
    log_message("IPC initialized (shared memory + semaphores)");

    // 3. Fork + exec captain
    printf("Starting port captain...\n");
    log_message("Forking port captain process");

    pid_t captain_pid = fork();

    if (captain_pid == -1) {
        perror("fork");
        log_message("ERROR: Failed to fork captain process");
        ipc_cleanup();
        logger_cleanup();
        return EXIT_FAILURE;
    }

    if (captain_pid == 0) {
        // Proces potomny - uruchom captain
        execl("./captain", "captain", NULL);

        // Jeśli execl się nie powiódł
        perror("execl captain");
        exit(EXIT_FAILURE);
    }

    // Proces rodzicielski - czeka na kapitana
    log_message("Port captain process started");
    printf("Port captain running (PID: %d)...\n", captain_pid);

    int status;
    waitpid(captain_pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("Port captain finished with status: %d\n", WEXITSTATUS(status));
        log_message("Port captain finished successfully");
    } else {
        log_message("WARNING: Port captain terminated abnormally");
    }

    // 4. Cleanup
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