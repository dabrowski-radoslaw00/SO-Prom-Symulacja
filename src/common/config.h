#ifndef CONFIG_H
#define CONFIG_H

/* ========================================
 * PODSTAWOWA KONFIGURACJA SYSTEMU
 * ======================================== */

/* === KLUCZE IPC === */
#define SHM_KEY 0x1234              // Klucz dla shared memory
#define SEM_KEY 0x5678              // Klucz dla zestawu semaforów

/* === SEMAFORY === */
#define SEM_SHM_MUTEX 0             // Mutex dla dostępu do shared memory
#define NUM_SEMAPHORES 1            // Na razie tylko jeden semafor

/* === PRAWA DOSTĘPU === */
#define IPC_PERMS 0600              // rw------- (minimalne prawa)

/* === PARAMETRY PASAŻERÓW === */
#define MAX_LUGGAGE_WEIGHT 23       // Mp - Maksymalny ciężar bagażu (kg)

/* Płeć */
typedef enum {
 MALE = 0,
 FEMALE = 1
} Gender;

/* Typ pasażera */
typedef enum {
 REGULAR = 0,
 VIP = 1
} PassengerType;

/* Status pasażera */
typedef enum {
 STATUS_WAITING = 0,         // Czeka w systemie
 STATUS_COMPLETED = 1        // Zakończył podróż
} PassengerStatus;

#endif