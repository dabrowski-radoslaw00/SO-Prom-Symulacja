#ifndef IPC_H
#define IPC_H

#include "common/shared_state.h"

int ipc_init(void);

SystemState* ipc_get_state(void);

void ipc_lock(void);
void ipc_unlock(void);
int ipc_cleanup(void);

#endif