#include "common/config.h"
#include "common/shared_state.h"
#include "utils/passenger/random_passenger.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>

static int shm_id = -1;
static int sem_id = -1;
static SystemState *state = NULL;

static void lock_mutex(void) {
    struct sembuf op;
    op.sem_num = SEM_SHM_MUTEX;
    op.sem_op = -1;
    op.sem_flg = 0;

    if (semop(sem_id, &op, 1) == -1) {
        perror("[PASSENGER] semop lock");
        exit(EXIT_FAILURE);
    }
}

static void unlock_mutex(void) {
    struct sembuf op;
    op.sem_num = SEM_SHM_MUTEX;
    op.sem_op = 1;
    op.sem_flg = 0;

    if (semop(sem_id, &op, 1) == -1) {
        perror("[PASSENGER] semop unlock");
        exit(EXIT_FAILURE);
    }
}

static int register_passenger(void) {
    lock_mutex();

    int passenger_id = -1;
    for (int i = 0; i < MAX_PASSENGERS; i++) {
        if (!state->passengers[i].active) {
            passenger_id = i;
            state->passengers[i].id = i;
            state->passengers[i].pid = getpid();
            state->passengers[i].active = true;
            state->passengers[i].gender = gender;
            state->passengers[i].luggage_weight = luggage_weight;
            state->passengers[i].type = type;
            state->passengers[i].status = STATUS_WAITING;

            state->total_passengers++;
            break;
        }
    }

    unlock_mutex();
    return passenger_id;
}

static void unregister_passenger(int passenger_id) {
    if (passenger_id < 0 || passenger_id >= MAX_PASSENGERS) {
        return;
    }

    lock_mutex();

    if (state->passengers[passenger_id].active) {
        state->passengers[passenger_id].active = false;
        state->passengers[passenger_id].status = STATUS_COMPLETED;
        state->total_passengers--;
    }

    unlock_mutex();
}

int main(void) {
    pid_t my_pid = getpid();

    PassengerAttributes attr = random_passenger_attributes();

    const char *gender_str = (attr.gender == MALE) ? "MALE" : "FEMALE";
    const char *type_str = (attr.type == VIP) ? "VIP" : "REGULAR";

    printf("[PASSENGER-%d] Starting... [%s, %dkg, %s]\n",
           my_pid, gender_str, attr.luggage_weight, type_str);

    shm_id = shmget(SHM_KEY, sizeof(SystemState), IPC_PERMS);
    if (shm_id == -1) {
        perror("[PASSENGER] shmget");
        return EXIT_FAILURE;
    }

    state = (SystemState*)shmat(shm_id, NULL, 0);
    if (state == (void*)-1) {
        perror("[PASSENGER] shmat");
        return EXIT_FAILURE;
    }

    sem_id = semget(SEM_KEY, NUM_SEMAPHORES, IPC_PERMS);
    if (sem_id == -1) {
        perror("[PASSENGER] semget");
        shmdt(state);
        return EXIT_FAILURE;
    }

    int my_id = register_passenger(attr.gender, attr.luggage_weight, attr.type);
    if (my_id == -1) {
        printf("[PASSENGER-%d] ERROR: No space available\n", my_pid);
        shmdt(state);
        return EXIT_FAILURE;
    }

    printf("[PASSENGER-%d] Registered with ID: %d\n", my_pid, my_id);

    if (attr.luggage_weight > MAX_LUGGAGE_WEIGHT) {
        printf("[PASSENGER-%d] ❌ Luggage too heavy (%dkg > %dkg) - REJECTED\n",
               my_pid, attr.luggage_weight, MAX_LUGGAGE_WEIGHT);
        printf("[PASSENGER-%d] Returning to check-in hall...\n", my_pid);
        sleep(1);
        unregister_passenger(my_id);
        shmdt(state);
        return EXIT_SUCCESS;
    } else {
        printf("[PASSENGER-%d] ✓ Luggage OK (%dkg <= %dkg)\n",
               my_pid, attr.luggage_weight, MAX_LUGGAGE_WEIGHT);
    }

    printf("[PASSENGER-%d] Waiting in hall...\n", my_pid);
    sleep(2);

    printf("[PASSENGER-%d] Going through check-in...\n", my_pid);
    sleep(1);

    printf("[PASSENGER-%d] Journey completed\n", my_pid);

    unregister_passenger(my_id);
    printf("[PASSENGER-%d] Unregistered\n", my_pid);

    if (shmdt(state) == -1) {
        perror("[PASSENGER] shmdt");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}