/*
 * mock_dungeon.c - CECS 326 Lab 2
 * MOCK SIMULATOR FOR LOCAL TESTING ONLY
 * Compiles into 'dungeon.o' to emulate your professor's secret binary.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <math.h>
#include <time.h>

#include "dungeon_info.h"

// We expect game.c to have already set up shared memory and semaphores!
void RunDungeon(pid_t wizard, pid_t rogue, pid_t barbarian) {
    printf("\n[Mock Dungeon] Engine successfully attached to process trees.\n");
    srand(time(NULL));

    // Connect to the shared memory segment initialized by game.c
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("[Mock Dungeon] Error attaching to shared memory");
        return;
    }

    struct Dungeon *dungeon = (struct Dungeon*) mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("[Mock Dungeon] Mmap linkage error");
        close(shm_fd);
        return;
    }

    // Set up the secret variables our players have to solve
    dungeon->enemy.health = 100;
    
    // Secret cipher setup ("T" = shift 84). Raw text: "Open Sesame"
    // Caesar cipher requires space inside the destination arrays
    snprintf(dungeon->barrier.spell, sizeof(dungeon->barrier.spell), "Ties%%Le{sfe"); 

    // Target lock location for Rogue (Random float between 0 and MAX_PICK_ANGLE)
    float secret_lock_angle = ((float)rand() / (float)RAND_MAX) * MAX_PICK_ANGLE;
    dungeon->trap.locked = true;

    printf("[Mock Dungeon] Systems calibrated. Secret Lock Target: %.2f\n", secret_lock_angle);

    // Run the standard game loops
    for (int round = 1; round <= NUM_ROUNDS && dungeon->running; round++) {
        printf("\n--- Dungeon Round %d ---\n", round);

        // 1. Barbarian Phase
        if (ALLOW_BARBARIAN) {
            dungeon->barbarian.attack = -1; // reset register
            kill(barbarian, DUNGEON_SIGNAL);
            sleep(SECONDS_TO_ATTACK);
            if (dungeon->barbarian.attack == dungeon->enemy.health) {
                printf("[Mock Dungeon] Barbarian HIT! (Dealt %d damage)\n", dungeon->barbarian.attack);
            } else {
                printf("[Mock Dungeon] Barbarian MISSED or desynced.\n");
            }
        }

        // 2. Wizard Phase
        if (ALLOW_WIZARD) {
            kill(wizard, DUNGEON_SIGNAL);
            sleep(SECONDS_TO_GUESS_BARRIER);
            // Quick evaluation check
            if (dungeon->wizard.spell[0] != '?' && dungeon->wizard.spell[0] != '\0') {
                printf("[Mock Dungeon] Wizard Barrier counter-measure evaluated: %s\n", dungeon->wizard.spell);
            }
        }

        // 3. Rogue Phase
        if (ALLOW_ROGUE && dungeon->trap.locked) {
            kill(rogue, DUNGEON_SIGNAL);
            
            // The dungeon ticks at high-frequency increments inside the window
            long total_ticks = (SECONDS_TO_PICK * 1000000) / TIME_BETWEEN_ROGUE_TICKS;
            for (long t = 0; t < total_ticks && dungeon->trap.locked; t++) {
                usleep(TIME_BETWEEN_ROGUE_TICKS);

                // Check where the Rogue positioned their pick tool
                float delta = secret_lock_angle - dungeon->rogue.pick;

                if (fabs(delta) <= LOCK_THRESHOLD) {
                    dungeon->trap.direction = '-';
                    dungeon->trap.locked = false;
                } else if (delta > 0) {
                    dungeon->trap.direction = 'u'; // tell rogue to guess higher
                } else {
                    dungeon->trap.direction = 'd'; // tell rogue to guess lower
                }
            }
            if (!dungeon->trap.locked) {
                printf("[Mock Dungeon] Rogue successfully bypassed the locking pins!\n");
            } else {
                printf("[Mock Dungeon] Rogue timed out on the trap mechanism.\n");
            }
        }

        // Progress enemy state degradation tracking
        dungeon->enemy.health -= 15;
        if (dungeon->enemy.health <= 0) dungeon->enemy.health = 5;
    }

    // 4. Endgame: The Semaphore Vault Room Challenge
    printf("\n[Mock Dungeon] Approaching Final Treasure Room Door!\n");
    
    // Prime the hidden vault slots with distinct data pieces
    dungeon->treasure[0] = 'G';
    dungeon->treasure[1] = 'O';
    dungeon->treasure[2] = 'L';
    dungeon->treasure[3] = 'D';
    
    // Clear spoils safely
    for(int i=0; i<4; i++) dungeon->spoils[i] = '\0';

    // Broadcast the named semaphore signals across the active party structure
    kill(barbarian, SEMAPHORE_SIGNAL);
    kill(wizard, SEMAPHORE_SIGNAL);
    kill(rogue, SEMAPHORE_SIGNAL);

    printf("[Mock Dungeon] Checking lever lock placements...\n");
    
    // Look up existing semaphores to see if party held them down
    sem_t *lever1 = sem_open(dungeon_lever_one, 0);
    sem_t *lever2 = sem_open(dungeon_lever_two, 0);

    // We check if the characters down'd them. sem_trywait will fail if they are locked/held!
    int tick_count = 0;
    while (tick_count < 4 && dungeon->running) {
        // If trywait fails, it means a character is holding it (value <= 0). Success!
        if (sem_trywait(lever1) != 0 && sem_trywait(lever2) != 0) {
            printf("[Mock Dungeon] Safe Tick %d: Both levers verified SECURED by party.\n", tick_count + 1);
            tick_count++;
        } else {
            printf("[Mock Dungeon] ALERT: Levers dropped! The heavy stone vault door is collapsing!\n");
            // If trywait succeeded, clean up the accidental decrement
            sem_post(lever1);
            sem_post(lever2);
        }
        sleep(1);
    }

    // Give the Rogue a brief moment to copy variables out before closing context
    sleep(1);

    // Final evaluation check
    printf("\n========================================\n");
    printf("[Mock Dungeon] Simulation Concluded.\n");
    printf("Final Spoils Array Content: %c%c%c%c\n", 
           dungeon->spoils[0], dungeon->spoils[1], dungeon->spoils[2], dungeon->spoils[3]);
    printf("========================================\n");

    // Detach context cleanly
    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);
}
