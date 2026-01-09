#include "common/config.h"
#include "common/shared_state.h"
#include "utils/logger/colors.h"
#include "utils/logger/logger.h"
#include "ipc/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

static int shm_id = -1;
static int sem_id = -1;
static SystemState *state = NULL;

static void lock_mutex(void) {
    struct sembuf op;
    op.sem_num = SEM_SHM_MUTEX;
    op.sem_op = -1;
    op.sem_flg = 0;

    if (semop(sem_id, &op, 1) == -1) {
        perror("[FERRY] semop lock");
        exit(EXIT_FAILURE);
    }
}

static void unlock_mutex(void) {
    struct sembuf op;
    op.sem_num = SEM_SHM_MUTEX;
    op.sem_op = 1;
    op.sem_flg = 0;

    if (semop(sem_id, &op, 1) == -1) {
        perror("[FERRY] semop unlock");
        exit(EXIT_FAILURE);
    }
}

static int register_ferry(int max_luggage_weight) {
    lock_mutex();

    int ferry_id = -1;
    for (int i = 0; i < MAX_FERRIES; i++) {
        if (!state->ferries[i].active) {
            ferry_id = i;
            state->ferries[i].id = i;
            state->ferries[i].pid = getpid();
            state->ferries[i].active = true;
            state->ferries[i].status = FERRY_IDLE;
            state->ferries[i].num_passengers = 0;
            state->ferries[i].max_luggage_weight = max_luggage_weight;
            state->ferries[i].trips_completed = 0;
            state->ferries[i].last_departure = 0;

            for (int j = 0; j < FERRY_CAPACITY; j++) {
                state->ferries[i].passenger_ids[j] = -1;
            }

            state->total_ferries++;
            break;
        }
    }

    unlock_mutex();
    return ferry_id;
}

static void unregister_ferry(int ferry_id) {
    if (ferry_id < 0 || ferry_id >= MAX_FERRIES) {
        return;
    }

    lock_mutex();

    if (state->ferries[ferry_id].active) {
        state->ferries[ferry_id].active = false;
        state->total_ferries--;
    }

    unlock_mutex();
}

static void set_ferry_status(int ferry_id, FerryStatus new_status) {
    lock_mutex();
    state->ferries[ferry_id].status = new_status;
    unlock_mutex();
}

int main(void) {
    pid_t my_pid = getpid();

    // Losowy limit bagażu dla tego promu (20-30 kg)
    srand(time(NULL) ^ my_pid);
    int max_luggage = 20 + (rand() % 11);  // 20-30 kg

    printf("%s[FERRY-%d]%s Starting... [Max luggage: %s%dkg%s]\n",
           COLOR_BRIGHT_CYAN, my_pid, COLOR_RESET,
           COLOR_YELLOW, max_luggage, COLOR_RESET);

    shm_id = shmget(SHM_KEY, sizeof(SystemState), IPC_PERMS);
    if (shm_id == -1) {
        perror("[FERRY] shmget");
        logger_cleanup();
        return EXIT_FAILURE;
    }

    state = (SystemState*)shmat(shm_id, NULL, 0);
    if (state == (void*)-1) {
        perror("[FERRY] shmat");
        logger_cleanup();
        return EXIT_FAILURE;
    }

    sem_id = semget(SEM_KEY, NUM_SEMAPHORES, IPC_PERMS);
    if (sem_id == -1) {
        perror("[FERRY] semget");
        shmdt(state);
        logger_cleanup();
        return EXIT_FAILURE;
    }

    int my_id = register_ferry(max_luggage);
    if (my_id == -1) {
        printf("%s[FERRY-%d]%s ERROR: No space for ferry\n",
               C_ERROR, my_pid, COLOR_RESET);
        shmdt(state);
        logger_cleanup();
        return EXIT_FAILURE;
    }

    printf("%s[FERRY-%d]%s Registered as Ferry #%s%d%s\n",
           COLOR_BRIGHT_CYAN, my_pid, COLOR_RESET,
           STYLE_BOLD, my_id, COLOR_RESET);

    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg),
             "Ferry #%d (PID:%d) registered [capacity:%d, max_luggage:%dkg]",
             my_id, my_pid, FERRY_CAPACITY, max_luggage);

    printf("%s[FERRY-%d]%s Ready to accept passengers (0/%d)...\n",
           COLOR_BRIGHT_CYAN, my_pid, COLOR_RESET, FERRY_CAPACITY);

    set_ferry_status(my_id, FERRY_BOARDING);

    usleep(100000);  // 100ms

    time_t boarding_start = time(NULL);
    const int BOARDING_TIME_LIMIT = 8;  // Max 8 sekund na boarding

    while (1) {
        sleep(1);

        if (ipc_is_ferry_full(my_id)) {
            printf("%s[FERRY-%d] ✓ Ferry is FULL (%d/%d passengers)!%s\n",
                   C_SUCCESS, my_pid, FERRY_CAPACITY, FERRY_CAPACITY, COLOR_RESET);
            break;
        }

        lock_mutex();
        int current_passengers = state->ferries[my_id].num_passengers;
        unlock_mutex();

        printf("%s[FERRY-%d]%s Waiting for passengers... (%d/%d)\n",
               COLOR_BRIGHT_CYAN, my_pid, COLOR_RESET,
               current_passengers, FERRY_CAPACITY);

        time_t now = time(NULL);
        if (difftime(now, boarding_start) >= BOARDING_TIME_LIMIT) {
            printf("%s[FERRY-%d]%s Boarding time expired, departing with %d passengers\n",
                   C_WARNING, my_pid, COLOR_RESET, current_passengers);
            break;
        }
    }

    lock_mutex();
    int num_passengers = state->ferries[my_id].num_passengers;
    printf("%s[FERRY-%d]%s Passenger manifest:%s\n",
           STYLE_BOLD, my_pid, COLOR_RESET, COLOR_RESET);

    for (int i = 0; i < num_passengers; i++) {
        int pass_id = state->ferries[my_id].passenger_ids[i];
        if (pass_id >= 0 && pass_id < MAX_PASSENGERS) {
            PassengerInfo *p = &state->passengers[pass_id];
            const char *gender_str = (p->gender == MALE) ? "M" : "F";
            const char *type_str = (p->type == VIP) ? "VIP" : "REG";

            printf("  - Passenger #%d [%s, %dkg/%dkg, %s]\n",
         pass_id, gender_str, p->luggage_weight,
         state->ferries[my_id].max_luggage_weight, type_str);
        }
    }

    unlock_mutex();

    printf("%s[FERRY-%d]%s Waiting for gangway to clear...%s\n",
           C_INFO, my_pid, COLOR_RESET, COLOR_RESET);

    int gangway_wait = 0;
    const int MAX_GANGWAY_WAIT = 10;

    while (!ipc_is_gangway_empty() && gangway_wait < MAX_GANGWAY_WAIT) {
        printf("%s[FERRY-%d]%s Gangway not empty (%d passengers), waiting...%s\n",
               C_WARNING, my_pid, COLOR_RESET, ipc_get_gangway_count(), COLOR_RESET);
        sleep(1);
        gangway_wait++;
    }

    if (!ipc_is_gangway_empty()) {
        printf("%s[FERRY-%d]%s WARNING: Departing with passengers still on gangway!%s\n",
               C_WARNING, my_pid, COLOR_RESET, COLOR_RESET);
    } else {
        printf("%s[FERRY-%d]%s Gangway clear%s\n",
               C_SUCCESS, my_pid, COLOR_RESET, COLOR_RESET);
    }

    printf("%s[FERRY-%d] ⛴️  Departing...%s\n",
           COLOR_BRIGHT_GREEN, my_pid, COLOR_RESET);

    set_ferry_status(my_id, FERRY_SAILING);

    lock_mutex();
    state->ferries[my_id].last_departure = time(NULL);
    state->ferries[my_id].trips_completed++;
    unlock_mutex();

    snprintf(log_msg, sizeof(log_msg),
             "Ferry #%d departed (trip #%d)",
             my_id, 1);
    log_message(log_msg);

    sleep(FERRY_TRIP_TIME);

    printf("%s[FERRY-%d] ⚓ Returned to port%s\n",
           COLOR_BRIGHT_CYAN, my_pid, COLOR_RESET);

    set_ferry_status(my_id, FERRY_RETURNING);
    sleep(1);

    snprintf(log_msg, sizeof(log_msg),
             "Ferry #%d returned to port",
             my_id);
    log_message(log_msg);

    unregister_ferry(my_id);
    printf("%s[FERRY-%d]%s Unregistered\n",
           STYLE_DIM, my_pid, COLOR_RESET);

    if (shmdt(state) == -1) {
        perror("[FERRY] shmdt");
        return EXIT_FAILURE;
    }


    logger_cleanup();
    return EXIT_SUCCESS;
}