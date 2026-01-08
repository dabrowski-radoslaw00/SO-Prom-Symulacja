#ifndef IPC_H
#define IPC_H

#include "common/shared_state.h"

int ipc_init(void);
int ipc_attach(void);


SystemState* ipc_get_state(void);

void ipc_lock(void);
void ipc_unlock(void);
int ipc_cleanup(void);

int ipc_register_passenger(pid_t pid, Gender gender, int luggage_weight, PassengerType type);
void ipc_unregister_passenger(int passenger_id);

int ipc_register_ferry(pid_t pid, int max_luggage_weight);
void ipc_unregister_ferry(int ferry_id);

int ipc_register_ferry(pid_t pid, int max_luggage_weight);
void ipc_unregister_ferry(int ferry_id);
int ipc_find_available_ferry(int luggage_weight);
int ipc_board_ferry(int ferry_id, int passenger_id);
bool ipc_is_ferry_full(int ferry_id);
int ipc_get_ferry_max_luggage(int ferry_id);

#endif