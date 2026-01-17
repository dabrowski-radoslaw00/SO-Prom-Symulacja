/**
 * @file test_logger_sync.c
 * @brief Test synchronizacji loggera - wiele procesów loguje jednocześnie.
 * 
 * Uruchomienie:
 *   ./test_logger_sync
 * 
 * Po zakończeniu sprawdź logs/system.log - każda linia powinna być kompletna.
 */

#include "utils/logger/logger.h"
#include "ipc/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define NUM_CHILD_PROCESSES 10
#define LOGS_PER_PROCESS 20

int main(void) {
    printf("=== Logger Synchronization Test ===\n\n");
    
    // Inicjalizacja IPC (tworzy semafory)
    printf("Initializing IPC...\n");
    if (ipc_init() == -1) {
        fprintf(stderr, "Failed to initialize IPC\n");
        return EXIT_FAILURE;
    }
    
    // Inicjalizacja loggera w procesie głównym
    printf("Initializing logger...\n");
    if (logger_init() == -1) {
        fprintf(stderr, "Failed to initialize logger\n");
        ipc_cleanup();
        return EXIT_FAILURE;
    }
    
    log_message("=== LOGGER SYNC TEST START ===");
    
    printf("Creating %d child processes, each will log %d messages...\n\n",
           NUM_CHILD_PROCESSES, LOGS_PER_PROCESS);
    
    pid_t pids[NUM_CHILD_PROCESSES];
    
    // Tworzenie procesów potomnych
    for (int i = 0; i < NUM_CHILD_PROCESSES; i++) {
        pids[i] = fork();
        
        if (pids[i] == -1) {
            perror("fork");
            continue;
        }
        
        if (pids[i] == 0) {
            // Proces potomny
            
            // Reinicjalizacja loggera w potomku (dołączenie do semaforów)
            if (logger_init() == -1) {
                exit(EXIT_FAILURE);
            }
            
            // Logowanie wielu wiadomości
            for (int j = 0; j < LOGS_PER_PROCESS; j++) {
                char msg[256];
                snprintf(msg, sizeof(msg), 
                         "Process %d (PID:%d) - Message %d of %d - ABCDEFGHIJKLMNOPQRSTUVWXYZ",
                         i, getpid(), j + 1, LOGS_PER_PROCESS);
                log_message(msg);
                
                // Krótka pauza aby zwiększyć szansę na kolizje
                usleep(1000 + (rand() % 5000));
            }
            
            logger_cleanup();
            exit(EXIT_SUCCESS);
        }
    }
    
    // Proces główny czeka na wszystkie potomki
    printf("Waiting for child processes to finish...\n");
    for (int i = 0; i < NUM_CHILD_PROCESSES; i++) {
        if (pids[i] > 0) {
            int status;
            waitpid(pids[i], &status, 0);
            printf("  Child %d (PID:%d) finished\n", i, pids[i]);
        }
    }
    
    log_message("=== LOGGER SYNC TEST END ===");
    
    printf("\n=== Test completed! ===\n");
    printf("Check logs/system.log for results.\n");
    printf("Each line should be complete (no mixed/corrupted entries).\n\n");
    
    printf("Expected: %d log entries from children + 2 from main\n",
           NUM_CHILD_PROCESSES * LOGS_PER_PROCESS);
    
    // Cleanup
    logger_cleanup();
    ipc_cleanup();
    
    return EXIT_SUCCESS;
}
