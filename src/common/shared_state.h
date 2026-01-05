#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include "config.h"
#include <stdbool.h>
#include <sys/types.h>

#define MAX_PASSENGERS 10

typedef struct {
    int id;
    pid_t pid;
    bool active;
    Gender gender;
    int luggage_weight;
    PassengerType type;
    PassengerStatus status;
} PassengerInfo;

typedef struct {
    bool system_running;
    int total_passengers;
    PassengerInfo passengers[MAX_PASSENGERS];
} SystemState;

#endif