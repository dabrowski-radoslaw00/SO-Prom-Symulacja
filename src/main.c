#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
   int num_passengers = 3;

   printf("Ferry Simulation - Starting...\n");
   printf("Creating %d passengers\n\n", num_passengers);

   for (int i = 0; i < num_passengers; i++) {
      pid_t pid = fork();

      if (pid == 0) {
         printf("Passenger %d: Arrived at port (PID: %d)\n", i, getpid());
         sleep(1 + i);
         printf("Passenger %d: Leaving port\n", i);
         exit(0);
      }

      printf("Parent: Created passenger %d (PID: %d)\n", i, pid);
   }

   printf("\nParent: Waiting for all passengers...\n");
   for (int i = 0; i < num_passengers; i++) {
      wait(NULL);
      printf("Parent: One passenger finished (%d/%d)\n", i+1, num_passengers);
   }

   printf("\nSimulation finished!\n");
   return 0;
}