/*
 * wizard.c - CECS 326 Lab 2
 * Character Process: Wizard (FIXED VERSION)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <ctype.h>
#include <string.h>

#include "dungeon_info.h"

struct Dungeon *dungeon = NULL;

/*
 * Fixed Caesar Cipher Decryption Loop
 */
void decode_caesar(const char *cipher, char *output) {
    if (cipher[0] == '\0') {
        output[0] = '\0';
        return;
    }

    // The key shift value is determined explicitly by the value of the first character
    int shift = cipher[0]; 
    int i = 1;  // FIX 1: Start at index 1 to completely skip printing the key byte into our answer!
    int out_idx = 0;

    while (cipher[i] != '\0') {
        char c = cipher[i];

        if (isalpha((unsigned char)c)) {
            if (isupper((unsigned char)c)) {
                output[out_idx] = ((c - 'A' - shift) % 26 + 26) % 26 + 'A';
            } else {
                output[out_idx] = ((c - 'a' - shift) % 26 + 26) % 26 + 'a';
            }
        } else {
            // FIX 2: Ensure space characters (' ') and punctuation pass through completely untouched
            output[out_idx] = c; 
        }
        i++;
        out_idx++;
    }
    output[out_idx] = '\0'; // Properly append string null terminator boundary
}

void handle_signals(int sig) {
    if (dungeon == NULL) return;

    if (sig == DUNGEON_SIGNAL) {
        printf("[Wizard] Magic barrier field detected. Deciphering glyphs...\n");
        char decoded[SPELL_BUFFER_SIZE + 1] = {0};
        
        decode_caesar(dungeon->barrier.spell, decoded);
        
        // Use strncpy to safely copy data and clear the truncation warning
        strncpy(dungeon->wizard.spell, decoded, sizeof(dungeon->wizard.spell) - 1);
        dungeon->wizard.spell[sizeof(dungeon->wizard.spell) - 1] = '\0';
        
        printf("[Wizard] Counter-spell focused: %s\n", dungeon->wizard.spell);
        
    } else if (sig == SEMAPHORE_SIGNAL) {
        printf("[Wizard] Semaphore signal caught! Securing Lever 2.\n");
        
        sem_t *lever2 = sem_open(dungeon_lever_two, 0);
        if (lever2 == SEM_FAILED) {
            perror("[Wizard] Failed to open lever 2 semaphore");
            return;
        }
        
        sem_wait(lever2);
        printf("[Wizard] Lever 2 successfully held down.\n");
        
        // Wait until Rogue populates the final index slot of spoils
        while (dungeon != NULL && dungeon->running && dungeon->spoils[3] == '\0') {
            usleep(10000);
        }
        
        sem_post(lever2);
        sem_close(lever2);
        printf("[Wizard] Lever 2 safely released.\n");
    }
}

int main() {
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("[Wizard] Failed to open shared memory");
        exit(1);
    }
    
    dungeon = (struct Dungeon*) mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("[Wizard] Failed to map shared memory");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    sigaction(DUNGEON_SIGNAL, &sa, NULL);
    sigaction(SEMAPHORE_SIGNAL, &sa, NULL);

    printf("[Wizard] Magical sensors calibrated.\n");

    while (dungeon->running) {
        pause();
    }

    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);
    return 0;
}
