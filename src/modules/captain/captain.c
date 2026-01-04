#include "utils/logger.h"
#include "ipc/ipc.h"
#include "common/shared_state.h"
#include "common/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

int main(void) {
    printf("[CAPTAIN] Port Captain starting...\n");
    
    int shm_id = shmget(SHM_KEY, sizeof(SystemState), IPC_PERMS);
    if (shm_id == -1) {
        perror("[CAPTAIN] shmget");
        return EXIT_FAILURE;
    }
    
    SystemState *state = (SystemState*)shmat(shm_id, NULL, 0);
    if (state == (void*)-1) {
        perror("[CAPTAIN] shmat");
        return EXIT_FAILURE;
    }

    int sem_id = semget(SEM_KEY, NUM_SEMAPHORES, IPC_PERMS);
    if (sem_id == -1) {
        perror("[CAPTAIN] semget");
        shmdt(state);
        return EXIT_FAILURE;
    }

    printf("[CAPTAIN] Connected to shared memory and semaphores\n");

    struct sembuf lock_op;
    lock_op.sem_num = SEM_SHM_MUTEX;
    lock_op.sem_op = -1;
    lock_op.sem_flg = 0;
    
    if (semop(sem_id, &lock_op, 1) == -1) {
        perror("[CAPTAIN] semop lock");
        shmdt(state);
        return EXIT_FAILURE;
    }
    
    printf("[CAPTAIN] System running: %s\n", 
           state->system_running ? "YES" : "NO");
    
    struct sembuf unlock_op;
    unlock_op.sem_num = SEM_SHM_MUTEX;
    unlock_op.sem_op = 1;
    unlock_op.sem_flg = 0;
    
    if (semop(sem_id, &unlock_op, 1) == -1) {
        perror("[CAPTAIN] semop unlock");
        shmdt(state);
        return EXIT_FAILURE;
    }

    printf("[CAPTAIN] Managing port operations...\n");
    
    for (int i = 1; i <= 5; i++) {
        sleep(1);
        printf("[CAPTAIN] Working... (%d/5)\n", i);
    }

    printf("[CAPTAIN] Port Captain finished\n");

    if (shmdt(state) == -1) {
        perror("[CAPTAIN] shmdt");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}