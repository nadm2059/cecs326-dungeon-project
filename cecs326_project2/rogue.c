/*
 * rogue.c - CECS 326 Lab 2
 * Character Process: Rogue
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

void handle_signals(int sig) {
    if (dungeon == NULL) return;

    if (sig == DUNGEON_SIGNAL) {
        printf("[Rogue] Trap mechanism encountered! Commencing binary lock-picking.\n");

        float low = 0.0f;
        float high = MAX_PICK_ANGLE;
        float guess;

        while (dungeon->trap.locked && dungeon->running) {
            guess = low + (high - low) / 2.0f;
            dungeon->rogue.pick = guess;

            // Recommendation check: Write temporary status to flag updates
            dungeon->trap.direction = 't';

            // Wait cleanly until the dungeon ticks and updates the direction character
            while (dungeon->trap.direction == 't' && dungeon->trap.locked && dungeon->running) {
                usleep(TIME_BETWEEN_ROGUE_TICKS);
            }

            if (dungeon->trap.direction == 'u') {
                low = guess; // The target value is higher
            } else if (dungeon->trap.direction == 'd') {
                high = guess; // The target value is lower
            } else if (dungeon->trap.direction == '-') {
                printf("[Rogue] Lock cleared! Final position: %.2f\n", guess);
                break;
            }
        }
        
    } else if (sig == SEMAPHORE_SIGNAL) {
        printf("[Rogue] Party levers secured! Sweeping treasure chamber...\n");

        int collected = 0;
        
        // Loop explicitly until all 4 characters are swept up
        while (collected < 4 && dungeon->running) {
            char item = dungeon->treasure[collected];
            
            // Check if a valid item character has manifested in the targeted layout array index
            if (item != '\0' && item != ' ' && item != '-') {
                dungeon->spoils[collected] = item;
                
                // Safe string manipulation printing technique: 
                // Because spoils has no room for a null terminator, print it character by character safely!
                printf("[Rogue] Collected artifact piece %d: %c\n", collected + 1, item);
                collected++;
            }
            usleep(20000); // Allow brief spacing between structural scan cycles
        }
        
        printf("[Rogue] Vault completely cleared out! Signals sent to party to drop levers.\n");
    }
}

int main() {
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("[Rogue] Failed to open shared memory");
        exit(1);
    }
    
    dungeon = (struct Dungeon*) mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("[Rogue] Failed to map shared memory");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    sigaction(DUNGEON_SIGNAL, &sa, NULL);
    sigaction(SEMAPHORE_SIGNAL, &sa, NULL);

    printf("[Rogue] Hidden in shadows, awaiting signals.\n");

    while (dungeon->running) {
        pause();
    }

    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);
    return 0;
}
