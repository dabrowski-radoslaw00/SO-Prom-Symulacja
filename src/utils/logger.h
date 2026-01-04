#ifndef LOGGER_H
#define LOGGER_H

int logger_init(void);

void log_message(const char *message);
void logger_cleanup(void);

#endif