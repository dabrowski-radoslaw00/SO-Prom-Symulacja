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

#define MAX_FERRIES 3               // N - Max liczba promów
#define FERRY_CAPACITY 10           // P - Pojemność promu
#define FERRY_TRIP_TIME 5          // Ti - Czas rejsu (sekundy)
#define GANGWAY_CAPACITY 5          // K - Pojemność trapu

#define NUM_SECURITY_STATIONS 3     // Liczba stanowisk kontroli
#define MAX_PEOPLE_PER_STATION 2    // Max osób na stanowisku
#define MAX_QUEUE_PASSES 3
#define SECURITY_CHECK_TIME 2       // Czas kontroli bezpieczeństwa (sekundy)

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
 STATUS_WAITING = 0,              // Czeka w systemie
 STATUS_IN_QUEUE = 1,             // W kolejce do kontroli
 STATUS_AT_SECURITY = 2,          // Na stanowisku kontroli
 STATUS_SECURITY_PASSED = 3,      // Przeszedł kontrolę
 STATUS_IN_WAITING_ROOM = 4,      // W poczekalni
 STATUS_ON_FERRY = 5,             // Na promie
 STATUS_COMPLETED = 6             // Zakończył podróż
} PassengerStatus;

/* Status promu */
typedef enum {
 FERRY_IDLE = 0,             // Bezczynny (nie przyjmuje pasażerów)
 FERRY_BOARDING = 1,         // Przyjmuje pasażerów
 FERRY_SAILING = 2,          // W trakcie rejsu
 FERRY_RETURNING = 3         // Wraca do portu
} FerryStatus;

#endif