#include "utils/logger.h"
#include "ipc/ipc.h"
#include "common/shared_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    printf("=== Ferry Ssimulation - testowaine ===\n\n");

    printf("1. Inicjalizacja loggere...\n");
    if (logger_init() == -1) {
        fprintf(stderr, "Bład inicjalozacji\n");
        return EXIT_FAILURE;
    }
    log_message("Zainicjalizoano");

    printf("2. Initializing IPC (shared memory + semaphores)...\n");
    if (ipc_init() == -1) {
        fprintf(stderr, "Failed to initialize IPC\n");
        log_message("ERROR: Failed to initialize IPC");
        logger_cleanup();
        return EXIT_FAILURE;
    }

    log_message("IPC zainicializoawało");

    printf("3. Zbieranie SHM...\n");

    SystemState *state = ipc_get_state();

    if (state == NULL) {
        fprintf(stderr, "Bład pobierania SHM\n");
        log_message("ERROR: Bład pobierania SHM");
        ipc_cleanup();
        logger_cleanup();
        return EXIT_FAILURE;
    }

    printf("4. testowanie lock/unlock...\n");
    ipc_lock();

    printf("  System działa: %s\n", state->system_running ? "TA" : "NIE");
    log_message(state->system_running ? "System działą" : "System przetał działac");

    ipc_unlock();

    printf("5. syumlacjia 3 sek pracy...\n");
    log_message("Pitu pitu...");
    sleep(3);

    printf("6. Smiana statusu systemu...\n");
    ipc_lock();
    state->system_running = false;
    log_message("System sie zatrzymal");
    ipc_unlock();


    printf("7. Czyszczenie...\n");
    log_message("Czyszczenie");

    if (ipc_cleanup() == -1) {
        fprintf(stderr, "blad czyszczenia IPC\n");
        log_message("blad czyszczenia IPC");
    } else {
        log_message("IPC wyczyszczono ");
    }

    logger_cleanup();

    printf("\n=== Koniec testu ===\n");

    return EXIT_SUCCESS;
}