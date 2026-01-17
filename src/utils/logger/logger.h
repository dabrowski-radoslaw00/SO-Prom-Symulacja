#ifndef LOGGER_H
#define LOGGER_H

#include "common/config.h"
#include <sys/types.h>


int logger_init(void);

void log_message(const char *message);
void logger_detach_semaphore(void);
void logger_cleanup(void);


void log_passenger_registered(int id, pid_t pid, Gender gender,
                               int luggage_weight, PassengerType type);
void log_passenger_rejected(pid_t pid, int luggage_weight, int limit);
void log_passenger_completed(int id, pid_t pid);

#endif