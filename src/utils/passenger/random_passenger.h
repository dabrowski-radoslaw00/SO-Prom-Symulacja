#ifndef RANDOM_PASSENGER_H
#define RANDOM_PASSENGER_H

#include "common/config.h"

typedef struct {
    Gender gender;
    int luggage_weight;
    PassengerType type;
} PassengerAttributes;

PassengerAttributes random_passenger_attributes(void);

#endif