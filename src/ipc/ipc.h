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

int ipc_join_security_queue(int passenger_id);
void ipc_leave_security_queue(int passenger_id);
int ipc_get_queue_position(int passenger_id);
int ipc_get_queue_size(void);

int ipc_assign_to_security_station(int passenger_id);
void ipc_leave_security_station(int passenger_id, int station_id);
int ipc_get_available_station_for_gender(Gender gender);
int ipc_complete_security_check(int passenger_id);
int ipc_pass_in_queue(int passenger_id);

int ipc_enter_gangway(int passenger_id, bool is_vip);
void ipc_exit_gangway(int passenger_id);
bool ipc_is_gangway_empty(void);
int ipc_get_gangway_count(void);

void ipc_register_ferry_pid(pid_t pid);
void ipc_unregister_ferry_pid(pid_t pid);
void ipc_register_passenger_pid(pid_t pid);
void ipc_unregister_passenger_pid(pid_t pid);

void ipc_send_signal_to_ferries(int sig);
void ipc_send_signal_to_passengers(int sig);
void ipc_send_signal_to_all(int sig);

bool ipc_is_boarding_allowed(void);
void ipc_set_boarding_allowed(bool allowed);
bool ipc_check_force_departure(void);
void ipc_set_force_departure(bool active);

bool ipc_all_passengers_finished(void);
void ipc_set_all_passengers_finished(bool finished);

int ipc_get_active_ferries_count(void);

#endif