#include "utils/passenger/random_passenger.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

static bool initialized = false;

static void ensure_initialized(void) {
    if (!initialized) {
        unsigned int seed = (time(NULL) ^ getpid());
        srand(seed);
        initialized = true;
    }
}

PassengerAttributes random_passenger_attributes(void) {
    ensure_initialized();
    
    PassengerAttributes attr;

    attr.gender = (rand() % 2 == 0) ? MALE : FEMALE;
    attr.luggage_weight = 5 + (rand() % 26);
    attr.type = (rand() % 100 < 10) ? VIP : REGULAR;
    
    return attr;
}
