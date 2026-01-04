#include "ipc/ipc.h"
#include "common/config.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static int shm_id = -1;
static int sem_id = -1;
static SystemState *state = NULL;

int ipc_init(void) {
    shm_id = shmget(SHM_KEY, sizeof(SystemState), IPC_CREAT | IPC_EXCL | IPC_PERMS);
    if (shm_id == -1) {
        perror("shmget");
        return -1;
    }
    
    state = (SystemState*)shmat(shm_id, NULL, 0);
    if (state == (void*)-1) {
        perror("shmat");
        shmctl(shm_id, IPC_RMID, NULL);
        return -1;
    }
    
    memset(state, 0, sizeof(SystemState));
    state->system_running = true;
    
    sem_id = semget(SEM_KEY, NUM_SEMAPHORES, IPC_CREAT | IPC_EXCL | IPC_PERMS);
    if (sem_id == -1) {
        perror("semget");
        shmdt(state);
        shmctl(shm_id, IPC_RMID, NULL);
        return -1;
    }
    
    union semun arg;
    arg.val = 1;
    if (semctl(sem_id, SEM_SHM_MUTEX, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        semctl(sem_id, 0, IPC_RMID);
        shmdt(state);
        shmctl(shm_id, IPC_RMID, NULL);
        return -1;
    }
    
    return 0;
}

SystemState* ipc_get_state(void) {
    return state;
}

void ipc_lock(void) {
    struct sembuf op;
    op.sem_num = SEM_SHM_MUTEX;
    op.sem_op = -1;
    op.sem_flg = 0;
    
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop lock");
        exit(EXIT_FAILURE);
    }
}

void ipc_unlock(void) {
    struct sembuf op;
    op.sem_num = SEM_SHM_MUTEX;
    op.sem_op = 1;
    op.sem_flg = 0;
    
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop unlock");
        exit(EXIT_FAILURE);
    }
}

int ipc_cleanup(void) {
    int ret = 0;
    
    if (state != NULL) {
        if (shmdt(state) == -1) {
            perror("shmdt");
            ret = -1;
        }
        state = NULL;
    }
    
    if (shm_id != -1) {
        if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
            perror("shmctl IPC_RMID");
            ret = -1;
        }
        shm_id = -1;
    }
    
    if (sem_id != -1) {
        if (semctl(sem_id, 0, IPC_RMID) == -1) {
            perror("semctl IPC_RMID");
            ret = -1;
        }
        sem_id = -1;
    }
    
    return ret;
}