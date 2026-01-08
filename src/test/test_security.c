#include "ipc/ipc.h"
#include "common/config.h"
#include "utils/logger/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

// Helper to clean up existing IPC
static void cleanup_existing_ipc(void) {
    int shm_id = shmget(SHM_KEY, sizeof(SystemState), IPC_PERMS);
    if (shm_id != -1) {
        shmctl(shm_id, IPC_RMID, NULL);
    }
    int sem_id = semget(SEM_KEY, NUM_SEMAPHORES, IPC_PERMS);
    if (sem_id != -1) {
        semctl(sem_id, 0, IPC_RMID);
    }
}

void print_station_status(SystemState *state) {
    printf("\n=== Station Status ===\n");
    for (int i = 0; i < NUM_SECURITY_STATIONS; i++) {
        SecurityStation *st = &state->security_stations[i];
        const char *gender_str = (st->station_gender == MALE) ? "MALE" : "FEMALE";
        printf("Station %d: %d/%d people, gender: %s, in_use: %s\n",
               i, st->num_people, MAX_PEOPLE_PER_STATION, gender_str,
               st->in_use ? "YES" : "NO");
        for (int j = 0; j < st->num_people; j++) {
            if (st->passenger_ids[j] >= 0) {
                PassengerInfo *p = &state->passengers[st->passenger_ids[j]];
                const char *type_str = (p->type == VIP) ? "VIP" : "REG";
                printf("  [%d] Passenger #%d (%s, %s)\n",
                       j, st->passenger_ids[j],
                       (p->gender == MALE) ? "M" : "F", type_str);
            }
        }
    }
    printf("Queue size: %d\n", state->security_queue_size);
    printf("=====================\n");
}

int main(void) {
    printf("=== Extended Security Control Test ===\n\n");

    cleanup_existing_ipc();

    if (logger_init() == -1) {
        fprintf(stderr, "Logger init failed\n");
        return EXIT_FAILURE;
    }

    if (ipc_init() == -1) {
        fprintf(stderr, "IPC init failed\n");
        logger_cleanup();
        return EXIT_FAILURE;
    }

    pid_t test_pid = getpid();
    srand(time(NULL) ^ test_pid);

    const int NUM_PASSENGERS = 15;  // Więcej pasażerów
    int passenger_ids[NUM_PASSENGERS];
    Gender genders[NUM_PASSENGERS];

    // Test 1: Register many passengers
    printf("Test 1: Registering %d passengers...\n", NUM_PASSENGERS);
    for (int i = 0; i < NUM_PASSENGERS; i++) {
        genders[i] = (rand() % 2 == 0) ? MALE : FEMALE;
        PassengerType type = (rand() % 10 < 2) ? VIP : REGULAR;  // 20% VIP
        int luggage = 5 + (rand() % 20);
        passenger_ids[i] = ipc_register_passenger(test_pid + i, genders[i], luggage, type);
        if (passenger_ids[i] >= 0) {
            const char *g = (genders[i] == MALE) ? "M" : "F";
            const char *t = (type == VIP) ? "VIP" : "REG";
            printf("  P%d: ID=%d [%s, %dkg, %s]\n",
                   i, passenger_ids[i], g, luggage, t);
        }
    }

    // Test 2: Join security queue
    printf("\nTest 2: Joining security queue...\n");
    int joined = 0;
    for (int i = 0; i < NUM_PASSENGERS; i++) {
        if (passenger_ids[i] >= 0) {
            int pos = ipc_join_security_queue(passenger_ids[i]);
            if (pos >= 0) {
                joined++;
            }
        }
    }
    printf("  %d passengers joined queue\n", joined);
    printf("  Queue size: %d\n", ipc_get_queue_size());

    SystemState *state = ipc_get_state();
    ipc_lock();
    print_station_status(state);
    ipc_unlock();

    // Test 3: Assign passengers to stations (process queue)
    printf("\nTest 3: Assigning passengers to stations...\n");
    int assigned = 0;
    int attempts = 0;
    const int MAX_ATTEMPTS = 50;

    while (attempts < MAX_ATTEMPTS) {
        int any_assigned = 0;

        for (int i = 0; i < NUM_PASSENGERS; i++) {
            if (passenger_ids[i] < 0) continue;

            // Check if passenger is in queue
            int queue_pos = ipc_get_queue_position(passenger_ids[i]);
            if (queue_pos >= 0) {
                // Try to assign
                int station = ipc_assign_to_security_station(passenger_ids[i]);
                if (station >= 0) {
                    assigned++;
                    any_assigned = 1;
                    const char *g = (genders[i] == MALE) ? "M" : "F";
                    printf("  P%d [%s] -> Station %d\n", i, g, station);
                }
            }
        }

        if (!any_assigned) {
            break;  // No more assignments possible
        }

        attempts++;

        // Show progress every 5 assignments
        if (assigned % 5 == 0 && assigned > 0) {
            ipc_lock();
            printf("  Progress: %d assigned, queue: %d\n",
                   assigned, state->security_queue_size);
            ipc_unlock();
        }
    }

    printf("\n  Total assigned: %d\n", assigned);

    ipc_lock();
    print_station_status(state);
    ipc_unlock();

    // Test 4: Verify gender segregation
    printf("\nTest 4: Verifying gender segregation...\n");
    int violations = 0;
    ipc_lock();
    for (int i = 0; i < NUM_SECURITY_STATIONS; i++) {
        SecurityStation *st = &state->security_stations[i];
        if (st->num_people > 0) {
            Gender station_gender = st->station_gender;
            for (int j = 0; j < st->num_people; j++) {
                int pid = st->passenger_ids[j];
                if (pid >= 0 && pid < MAX_PASSENGERS) {
                    if (state->passengers[pid].gender != station_gender) {
                        printf("  ✗ VIOLATION: Station %d has mixed genders!\n", i);
                        violations++;
                    }
                }
            }
        }
    }
    ipc_unlock();

    if (violations == 0) {
        printf("  ✓ All stations correctly segregated by gender\n");
    } else {
        printf("  ✗ Found %d gender segregation violations!\n", violations);
    }

    // Test 5: Verify station capacity limits
    printf("\nTest 5: Verifying station capacity limits...\n");
    int capacity_violations = 0;
    ipc_lock();
    for (int i = 0; i < NUM_SECURITY_STATIONS; i++) {
        SecurityStation *st = &state->security_stations[i];
        if (st->num_people > MAX_PEOPLE_PER_STATION) {
            printf("  ✗ VIOLATION: Station %d has %d people (max: %d)!\n",
                   i, st->num_people, MAX_PEOPLE_PER_STATION);
            capacity_violations++;
        }
    }
    ipc_unlock();

    if (capacity_violations == 0) {
        printf("  ✓ All stations respect capacity limits\n");
    } else {
        printf("  ✗ Found %d capacity violations!\n", capacity_violations);
    }


    // Test 5.5: Complete security checks
    printf("\nTest 5.5: Completing security checks...\n");
    int completed = 0;
    for (int i = 0; i < NUM_PASSENGERS; i++) {
        if (passenger_ids[i] < 0) continue;

        // Check if passenger is at security station
        ipc_lock();
        int station_id = state->passengers[passenger_ids[i]].security_station_id;
        PassengerStatus status = state->passengers[passenger_ids[i]].status;
        ipc_unlock();

        if (station_id >= 0 && status == STATUS_AT_SECURITY) {
            int result = ipc_complete_security_check(passenger_ids[i]);
            if (result == 0) {
                completed++;
                printf("  P%d completed security check\n", i);
            }
        }
    }

    printf("  Total completed: %d\n", completed);

    ipc_lock();
    printf("\nStatus after security checks:\n");
    for (int i = 0; i < NUM_PASSENGERS; i++) {
        if (passenger_ids[i] >= 0) {
            PassengerStatus s = state->passengers[passenger_ids[i]].status;
            const char *status_str;
            switch(s) {
                case STATUS_WAITING: status_str = "WAITING"; break;
                case STATUS_IN_QUEUE: status_str = "IN_QUEUE"; break;
                case STATUS_AT_SECURITY: status_str = "AT_SECURITY"; break;
                case STATUS_SECURITY_PASSED: status_str = "SECURITY_PASSED"; break;
                default: status_str = "UNKNOWN"; break;
            }
            printf("  P%d: %s\n", i, status_str);
        }
    }
    print_station_status(state);
    ipc_unlock();

    // Test 6: Process remaining queue (simulate stations freeing up)
    printf("\nTest 6: Processing remaining queue (simulating stations freeing up)...\n");

    // Free up some stations
    ipc_lock();
    int stations_freed = 0;
    for (int i = 0; i < NUM_SECURITY_STATIONS; i++) {
        SecurityStation *st = &state->security_stations[i];
        if (st->num_people > 0) {
            // Remove first person from each station
            int pid = st->passenger_ids[0];
            if (pid >= 0) {
                ipc_unlock();
                ipc_leave_security_station(pid, i);
                ipc_lock();
                stations_freed++;
            }
        }
    }
    ipc_unlock();

    printf("  Freed %d stations\n", stations_freed);

    // Try to assign remaining passengers
    int remaining_assigned = 0;
    for (int i = 0; i < NUM_PASSENGERS; i++) {
        if (passenger_ids[i] < 0) continue;

        int queue_pos = ipc_get_queue_position(passenger_ids[i]);
        if (queue_pos >= 0) {
            int station = ipc_assign_to_security_station(passenger_ids[i]);
            if (station >= 0) {
                remaining_assigned++;
            }
        }
    }

    printf("  Assigned %d more passengers after freeing stations\n", remaining_assigned);

    ipc_lock();
    print_station_status(state);
    ipc_unlock();

    // Summary
    printf("\n=== Test Summary ===\n");
    printf("Total passengers: %d\n", NUM_PASSENGERS);
    printf("Joined queue: %d\n", joined);
    printf("Assigned to stations: %d\n", assigned + remaining_assigned);
    printf("Remaining in queue: %d\n", ipc_get_queue_size());
    printf("Gender violations: %d\n", violations);
    printf("Capacity violations: %d\n", capacity_violations);

    if (violations == 0 && capacity_violations == 0) {
        printf("\n✓ All tests passed!\n");
    } else {
        printf("\n✗ Some tests failed!\n");
    }

    ipc_cleanup();
    logger_cleanup();

    return (violations == 0 && capacity_violations == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}