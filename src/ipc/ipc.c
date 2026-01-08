#include "ipc/ipc.h"
#include "common/config.h"
#include "utils/logger/logger.h"
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

    for (int i = 0; i < NUM_SECURITY_STATIONS; i++) {
        state->security_stations[i].id = i;
        state->security_stations[i].num_people = 0;
        state->security_stations[i].station_gender = MALE;
        state->security_stations[i].in_use = false;
        for (int j = 0; j < MAX_PEOPLE_PER_STATION; j++) {
            state->security_stations[i].passenger_ids[j] = -1;
        }
    }

    state->security_queue_size = 0;
    state->security_queue_head = 0;
    state->security_queue_tail = 0;
    for (int i = 0; i < MAX_SECURITY_QUEUE; i++) {
        state->security_queue[i] = -1;
    }

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


int ipc_attach(void) {
    shm_id = shmget(SHM_KEY, sizeof(SystemState), IPC_PERMS);
    if (shm_id == -1) {
        perror("shmget attach");
        return -1;
    }

    state = (SystemState*)shmat(shm_id, NULL, 0);
    if (state == (void*)-1) {
        perror("shmat attach");
        return -1;
    }

    sem_id = semget(SEM_KEY, NUM_SEMAPHORES, IPC_PERMS);
    if (sem_id == -1) {
        perror("semget attach");
        shmdt(state);
        state = NULL;
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
    op.sem_op = -1;  // P (down) - zmniejsz o 1
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

int ipc_register_passenger(pid_t pid, Gender gender, int luggage_weight, PassengerType type) {
    if (state == NULL) {
        return -1;
    }

    ipc_lock();

    int passenger_id = -1;
    for (int i = 0; i < MAX_PASSENGERS; i++) {
        if (!state->passengers[i].active) {
            passenger_id = i;
            state->passengers[i].id = i;
            state->passengers[i].pid = pid;
            state->passengers[i].active = true;
            state->passengers[i].gender = gender;
            state->passengers[i].luggage_weight = luggage_weight;
            state->passengers[i].type = type;
            state->passengers[i].status = STATUS_WAITING;
            state->passengers[i].ferry_id = -1;
            state->passengers[i].security_station_id = -1;
            state->passengers[i].queue_position = -1;
            state->passengers[i].times_passed = 0;
            state->passengers[i].in_security_queue = false;
            state->total_passengers++;
            break;
        }
    }

    ipc_unlock();

    return passenger_id;
}

void ipc_unregister_passenger(int passenger_id) {
    if (state == NULL || passenger_id < 0 || passenger_id >= MAX_PASSENGERS) {
        return;
    }

    ipc_lock();

    if (state->passengers[passenger_id].active) {
        state->passengers[passenger_id].active = false;
        state->total_passengers--;
    }

    ipc_unlock();
}

int ipc_register_ferry(pid_t pid, int max_luggage_weight) {
    if (state == NULL) {
        return -1;
    }

    ipc_lock();

    int ferry_id = -1;
    for (int i = 0; i < MAX_FERRIES; i++) {
        if (!state->ferries[i].active) {
            ferry_id = i;
            state->ferries[i].id = i;
            state->ferries[i].pid = pid;
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

    ipc_unlock();

    return ferry_id;
}

void ipc_unregister_ferry(int ferry_id) {
    if (state == NULL || ferry_id < 0 || ferry_id >= MAX_FERRIES) {
        return;
    }

    ipc_lock();

    if (state->ferries[ferry_id].active) {
        state->ferries[ferry_id].active = false;
        state->total_ferries--;
    }

    ipc_unlock();
}

int ipc_find_available_ferry(int luggage_weight) {
    if (state == NULL) {
        return -1;
    }

    ipc_lock();

    int ferry_id = -1;

    for (int i = 0; i < MAX_FERRIES; i++) {
        if (state->ferries[i].active &&
            state->ferries[i].status == FERRY_BOARDING &&
            state->ferries[i].num_passengers < FERRY_CAPACITY &&
            luggage_weight <= state->ferries[i].max_luggage_weight) {

            ferry_id = i;
            break;
        }
    }

    ipc_unlock();

    return ferry_id;
}

int ipc_board_ferry(int ferry_id, int passenger_id) {
    if (state == NULL ||
        ferry_id < 0 || ferry_id >= MAX_FERRIES ||
        passenger_id < 0 || passenger_id >= MAX_PASSENGERS) {
        return -1;
    }

    ipc_lock();

    if (!state->ferries[ferry_id].active ||
        state->ferries[ferry_id].status != FERRY_BOARDING ||
        state->ferries[ferry_id].num_passengers >= FERRY_CAPACITY) {
        ipc_unlock();
        return -1;
    }

    int slot = state->ferries[ferry_id].num_passengers;
    state->ferries[ferry_id].passenger_ids[slot] = passenger_id;
    state->ferries[ferry_id].num_passengers++;

    state->passengers[passenger_id].status = STATUS_ON_FERRY;
    state->passengers[passenger_id].ferry_id = ferry_id;

    ipc_unlock();

    return 0;
}

bool ipc_is_ferry_full(int ferry_id) {
    if (state == NULL || ferry_id < 0 || ferry_id >= MAX_FERRIES) {
        return false;
    }

    ipc_lock();

    bool is_full = (state->ferries[ferry_id].num_passengers >= FERRY_CAPACITY);

    ipc_unlock();

    return is_full;
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

int ipc_get_ferry_max_luggage(int ferry_id) {
    if (state == NULL || ferry_id < 0 || ferry_id >= MAX_FERRIES) {
        return -1;
    }

    ipc_lock();

    int max_luggage = -1;
    if (state->ferries[ferry_id].active) {
        max_luggage = state->ferries[ferry_id].max_luggage_weight;
    }

    ipc_unlock();

    return max_luggage;
}

int ipc_join_security_queue(int passenger_id) {
    if (state == NULL || passenger_id < 0 || passenger_id >= MAX_PASSENGERS) {
        return -1;
    }

    ipc_lock();

    if (state->passengers[passenger_id].in_security_queue) {
        ipc_unlock();
        return -1;
    }

    if (state->security_queue_size >= MAX_SECURITY_QUEUE) {
        ipc_unlock();
        return -2;
    }


    int tail = state->security_queue_tail;
    state->security_queue[tail] = passenger_id;
    state->security_queue_tail = (tail + 1) % MAX_SECURITY_QUEUE;
    state->security_queue_size++;

    state->passengers[passenger_id].in_security_queue = true;
    state->passengers[passenger_id].queue_position = state->security_queue_size - 1;
    state->passengers[passenger_id].status = STATUS_IN_QUEUE;

    int position = state->passengers[passenger_id].queue_position;

    ipc_unlock();

    return position;
}

void ipc_leave_security_queue(int passenger_id) {
    if (state == NULL || passenger_id < 0 || passenger_id >= MAX_PASSENGERS) {
        return;
    }

    ipc_lock();

    if (!state->passengers[passenger_id].in_security_queue) {
        ipc_unlock();
        return;
    }

    int found_index = -1;
    for (int i = 0; i < state->security_queue_size; i++) {
        int idx = (state->security_queue_head + i) % MAX_SECURITY_QUEUE;
        if (state->security_queue[idx] == passenger_id) {
            found_index = idx;
            break;
        }
    }

    if (found_index != -1) {

        int queue_pos = 0;
        for (int i = 0; i < state->security_queue_size; i++) {
            int idx = (state->security_queue_head + i) % MAX_SECURITY_QUEUE;
            if (idx == found_index) {
                continue;
            }
            int new_idx = (state->security_queue_head + queue_pos) % MAX_SECURITY_QUEUE;
            state->security_queue[new_idx] = state->security_queue[idx];
            queue_pos++;
        }

        state->security_queue_size--;
        state->security_queue_tail = (state->security_queue_tail - 1 + MAX_SECURITY_QUEUE) % MAX_SECURITY_QUEUE;


        for (int i = 0; i < state->security_queue_size; i++) {
            int idx = (state->security_queue_head + i) % MAX_SECURITY_QUEUE;
            int pid = state->security_queue[idx];
            if (pid >= 0 && pid < MAX_PASSENGERS) {
                state->passengers[pid].queue_position = i;
            }
        }
    }

    state->passengers[passenger_id].in_security_queue = false;
    state->passengers[passenger_id].queue_position = -1;

    ipc_unlock();
}

int ipc_get_queue_position(int passenger_id) {
    if (state == NULL || passenger_id < 0 || passenger_id >= MAX_PASSENGERS) {
        return -1;
    }

    ipc_lock();

    int position = -1;
    if (state->passengers[passenger_id].in_security_queue) {
        position = state->passengers[passenger_id].queue_position;
    }

    ipc_unlock();

    return position;
}

int ipc_get_queue_size(void) {
    if (state == NULL) {
        return -1;
    }

    ipc_lock();
    int size = state->security_queue_size;
    ipc_unlock();

    return size;
}

int ipc_get_available_station_for_gender(Gender gender) {
    if (state == NULL) {
        return -1;
    }

    ipc_lock();

    int station_id = -1;

    for (int i = 0; i < NUM_SECURITY_STATIONS; i++) {
        if (state->security_stations[i].station_gender == gender &&
            state->security_stations[i].num_people < MAX_PEOPLE_PER_STATION) {
            station_id = i;
            break;
        }
    }

    if (station_id == -1) {
        for (int i = 0; i < NUM_SECURITY_STATIONS; i++) {
            if (state->security_stations[i].num_people == 0) {
                station_id = i;
                break;
            }
        }
    }

    ipc_unlock();

    return station_id;
}

int ipc_assign_to_security_station(int passenger_id) {
    if (state == NULL || passenger_id < 0 || passenger_id >= MAX_PASSENGERS) {
        return -1;
    }

    ipc_lock();


    if (!state->passengers[passenger_id].in_security_queue) {
        ipc_unlock();
        return -1;
    }

    if (state->passengers[passenger_id].security_station_id >= 0) {
        ipc_unlock();
        return -2;
    }

    Gender passenger_gender = state->passengers[passenger_id].gender;
    int station_id = -1;

    for (int i = 0; i < NUM_SECURITY_STATIONS; i++) {
        SecurityStation *station = &state->security_stations[i];

        if (station->num_people == 0) {
            station_id = i;
            break;
        } else if (station->station_gender == passenger_gender &&
                   station->num_people < MAX_PEOPLE_PER_STATION) {
            station_id = i;
            break;
        }
    }

    if (station_id == -1) {
        ipc_unlock();
        return -3;
    }

    SecurityStation *station = &state->security_stations[station_id];

    if (station->num_people == 0) {
        station->station_gender = passenger_gender;
        station->in_use = true;
    }

    int slot = station->num_people;
    station->passenger_ids[slot] = passenger_id;
    station->num_people++;

    state->passengers[passenger_id].security_station_id = station_id;
    state->passengers[passenger_id].status = STATUS_AT_SECURITY;

    if (state->passengers[passenger_id].in_security_queue) {
        int found_index = -1;
        for (int i = 0; i < state->security_queue_size; i++) {
            int idx = (state->security_queue_head + i) % MAX_SECURITY_QUEUE;
            if (state->security_queue[idx] == passenger_id) {
                found_index = idx;
                break;
            }
        }

        if (found_index != -1) {

            int queue_pos = 0;
            for (int i = 0; i < state->security_queue_size; i++) {
                int idx = (state->security_queue_head + i) % MAX_SECURITY_QUEUE;
                if (idx == found_index) {
                    continue;
                }
                int new_idx = (state->security_queue_head + queue_pos) % MAX_SECURITY_QUEUE;
                state->security_queue[new_idx] = state->security_queue[idx];
                queue_pos++;
            }

            state->security_queue_size--;
            state->security_queue_tail = (state->security_queue_tail - 1 + MAX_SECURITY_QUEUE) % MAX_SECURITY_QUEUE;

            for (int i = 0; i < state->security_queue_size; i++) {
                int idx = (state->security_queue_head + i) % MAX_SECURITY_QUEUE;
                int pid = state->security_queue[idx];
                if (pid >= 0 && pid < MAX_PASSENGERS) {
                    state->passengers[pid].queue_position = i;
                }
            }
        }

        state->passengers[passenger_id].in_security_queue = false;
        state->passengers[passenger_id].queue_position = -1;
    }

    ipc_unlock();

    return station_id;
}

void ipc_leave_security_station(int passenger_id, int station_id) {
    if (state == NULL ||
        passenger_id < 0 || passenger_id >= MAX_PASSENGERS ||
        station_id < 0 || station_id >= NUM_SECURITY_STATIONS) {
        return;
    }

    ipc_lock();

    SecurityStation *station = &state->security_stations[station_id];

    int found_slot = -1;
    for (int i = 0; i < station->num_people; i++) {
        if (station->passenger_ids[i] == passenger_id) {
            found_slot = i;
            break;
        }
    }

    if (found_slot != -1) {
        for (int i = found_slot; i < station->num_people - 1; i++) {
            station->passenger_ids[i] = station->passenger_ids[i + 1];
        }
        station->num_people--;
        station->passenger_ids[station->num_people] = -1;

        if (station->num_people == 0) {
            station->in_use = false;
            station->station_gender = MALE;
        }
    }


    state->passengers[passenger_id].security_station_id = -1;

    ipc_unlock();
}

int ipc_complete_security_check(int passenger_id) {
    if (state == NULL || passenger_id < 0 || passenger_id >= MAX_PASSENGERS) {
        return -1;
    }

    ipc_lock();

    if (state->passengers[passenger_id].security_station_id < 0) {
        ipc_unlock();
        return -1;
    }

    if (state->passengers[passenger_id].status != STATUS_AT_SECURITY) {
        ipc_unlock();
        return -2;
    }

    int station_id = state->passengers[passenger_id].security_station_id;

    SecurityStation *station = &state->security_stations[station_id];
    int found_slot = -1;
    for (int i = 0; i < station->num_people; i++) {
        if (station->passenger_ids[i] == passenger_id) {
            found_slot = i;
            break;
        }
    }

    if (found_slot != -1) {
        for (int i = found_slot; i < station->num_people - 1; i++) {
            station->passenger_ids[i] = station->passenger_ids[i + 1];
        }
        station->num_people--;
        station->passenger_ids[station->num_people] = -1;


        if (station->num_people == 0) {
            station->in_use = false;
            station->station_gender = MALE;
        }
    }

    state->passengers[passenger_id].security_station_id = -1;
    state->passengers[passenger_id].status = STATUS_SECURITY_PASSED;

    ipc_unlock();

    return 0;
}