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

#endif /* CONFIG_H */