#include "logger.h"

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>

static FILE *log_file = NULL;

int logger_init(void) {

    struct stat st = {0};
    if (stat("logs", &st) == -1) {
        if (mkdir("logs", 0700) == -1) {
            perror("mkdir logs");
            return -1;
        }
    }


    log_file = fopen("logs/system.log", "a");
    if (log_file == NULL) {
        perror("fopen logs/system.log");
        return -1;
    }

    setlinebuf(log_file);
    
    return 0;
}

void log_message(const char *message) {
    if (log_file == NULL) {
        fprintf(stderr, "Logger not initialized\n");
        return;
    }

    time_t now;
    struct tm *timeinfo;
    char timestamp[64];
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    fprintf(log_file, "[%s] %s\n", timestamp, message);
    
    fprintf(stderr, "[%s] %s\n", timestamp, message);
}

void logger_cleanup(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}