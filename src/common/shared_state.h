#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include "config.h"
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#define MAX_PASSENGERS 10
#define MAX_SECURITY_QUEUE 50

typedef struct {
    int id;
    pid_t pid;
    bool active;
    Gender gender;
    int luggage_weight;
    PassengerType type;
    PassengerStatus status;
    int ferry_id;

    int security_station_id;
    int queue_position;
    int times_passed;
    bool in_security_queue;
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
    int id;
    int num_people;
    int passenger_ids[MAX_PEOPLE_PER_STATION];
    Gender station_gender;
    bool in_use;
} SecurityStation;

typedef struct {
    bool system_running;
    int total_passengers;
    PassengerInfo passengers[MAX_PASSENGERS];
    int total_ferries;
    FerryInfo ferries[MAX_FERRIES];
    SecurityStation security_stations[NUM_SECURITY_STATIONS];
    int security_queue[MAX_SECURITY_QUEUE];
    int security_queue_size;
    int security_queue_head;
    int security_queue_tail;
} SystemState;

#endif
