#include "utils/logger/logger.h"
#include "common/config.h"
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>

static FILE *log_file = NULL;
static int log_sem_id = -1;

static void logger_lock(void) {
    if (log_sem_id == -1) {
        return;
    }
    
    struct sembuf op;
    op.sem_num = SEM_LOG_MUTEX;
    op.sem_op = -1;
    op.sem_flg = 0;
    
    if (semop(log_sem_id, &op, 1) == -1) {
        perror("[LOGGER] semop lock");
    }
}

static void logger_unlock(void) {
    if (log_sem_id == -1) {
        return;
    }
    
    struct sembuf op;
    op.sem_num = SEM_LOG_MUTEX;
    op.sem_op = 1;
    op.sem_flg = 0;
    
    if (semop(log_sem_id, &op, 1) == -1) {
        perror("[LOGGER] semop unlock");
    }
}

int logger_init(void) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        fprintf(stderr, "Working directory: %s\n", cwd);
    }

    log_sem_id = semget(SEM_KEY, NUM_SEMAPHORES, IPC_PERMS);
    if (log_sem_id == -1) {
        fprintf(stderr, "[LOGGER] Warning: Could not attach to semaphores, logging without sync\n");
    }

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

    fprintf(stderr, "Log file: %s/logs/system.log\n", cwd);

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

    logger_lock();
    
    fprintf(log_file, "[%s] %s\n", timestamp, message);
    fflush(log_file);
    
    logger_unlock();

    fprintf(stderr, "[%s] %s\n", timestamp, message);
}

void logger_cleanup(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}

void log_passenger_registered(int id, pid_t pid, Gender gender,
                               int luggage_weight, PassengerType type) {
    const char *gender_str = (gender == MALE) ? "MALE" : "FEMALE";
    const char *type_str = (type == VIP) ? "VIP" : "REGULAR";

    char message[256];
    snprintf(message, sizeof(message),
             "Passenger #%d (PID:%d) [%s, %dkg, %s] registered",
             id, pid, gender_str, luggage_weight, type_str);

    log_message(message);
}

void log_passenger_rejected(pid_t pid, int luggage_weight, int limit) {
    char message[256];
    snprintf(message, sizeof(message),
             "Passenger (PID:%d) rejected - luggage too heavy (%dkg > %dkg)",
             pid, luggage_weight, limit);

    log_message(message);
}

void log_passenger_completed(int id, pid_t pid) {
    char message[128];
    snprintf(message, sizeof(message),
             "Passenger #%d (PID:%d) completed journey",
             id, pid);

    log_message(message);
}