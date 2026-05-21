/*
 * barbarian.c - CECS 326 Lab 2
 * Character Process: Barbarian
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#include "dungeon_info.h"

struct Dungeon *dungeon = NULL;

/* 
 * Dual-Purpose Signal Handler
 * Inspects incoming signals to route actions dynamically using header definitions.
 */
void handle_signals(int sig) {
    if (dungeon == NULL) return;

    if (sig == DUNGEON_SIGNAL) {
        printf("[Barbarian] Battle signal received! Attacking enemy health point.\n");
        // Copy enemy health directly into our attack register field
        dungeon->barbarian.attack = dungeon->enemy.health;
        
    } else if (sig == SEMAPHORE_SIGNAL) {
        printf("[Barbarian] Semaphore signal caught! Securing Lever 1.\n");
        
        // Open the exact named semaphore specified by your dungeon_info.h variable
        sem_t *lever1 = sem_open(dungeon_lever_one, 0);
        if (lever1 == SEM_FAILED) {
            perror("[Barbarian] Failed to open lever 1 semaphore");
            return;
        }
        
        // Lock/down the semaphore to hold open the door for the Rogue
        sem_wait(lever1);
        printf("[Barbarian] Lever 1 successfully held down.\n");
        
        // Wait until the Rogue finishes harvesting all 4 treasure characters
        // We look at index 3 of spoils because the array is size 4 (indices 0, 1, 2, 3)
        while (dungeon != NULL && dungeon->running && dungeon->spoils[3] == '\0') {
            usleep(10000); 
        }
        
        // Release the semaphore as requested by the lab description
        sem_post(lever1);
        sem_close(lever1);
        printf("[Barbarian] Lever 1 safely released.\n");
    }
}

int main() {
    // Open using the exact variable name provided in your header
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("[Barbarian] Failed to open shared memory");
        exit(1);
    }
    
    dungeon = (struct Dungeon*) mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("[Barbarian] Failed to map shared memory");
        exit(1);
    }

    // Set up standard POSIX signal routing configurations
    struct sigaction sa;
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Keeps system calls stable when signals interrupt
    
    sigaction(DUNGEON_SIGNAL, &sa, NULL);
    sigaction(SEMAPHORE_SIGNAL, &sa, NULL);

    printf("[Barbarian] Standing by in the dungeon.\n");

    while (dungeon->running) {
        pause(); 
    }

    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);
    return 0;
}
