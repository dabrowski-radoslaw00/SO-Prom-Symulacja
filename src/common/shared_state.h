#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <stdbool.h>

/* ========================================
 * MINIMALNY STAN SYSTEMU
 * ======================================== */

typedef struct {
    bool system_running;            // Czy system działa
} SystemState;

#endif /* SHARED_STATE_H */