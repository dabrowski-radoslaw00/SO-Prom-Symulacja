#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include "config.h"
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#define MAX_PASSENGERS 10

typedef struct {
    int id;
    pid_t pid;
    bool active;
    Gender gender;
    int luggage_weight;
    PassengerType type;
    PassengerStatus status;
    int ferry_id;
} PassengerInfo;

typedef struct {
    int id;
    pid_t pid;
    bool active;
    FerryStatus status;
    int num_passengers;
    int passenger_ids[FERRY_CAPACITY];
    int max_luggage_weight;
    int trips_completed;
    time_t last_departure;
} FerryInfo;

typedef struct {
    bool system_running;
    int total_passengers;
    PassengerInfo passengers[MAX_PASSENGERS];
    int total_ferries;
    FerryInfo ferries[MAX_FERRIES];
} SystemState;

#endif
