/*
 * game.c - CECS 326 Lab 2
 * Master Game Launcher Orchestrator
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <string.h>

#include "dungeon_info.h"

pid_t pid_barbarian, pid_wizard, pid_rogue;
int shm_fd = -1;
struct Dungeon *dungeon = NULL;
sem_t *lever1 = SEM_FAILED;
sem_t *lever2 = SEM_FAILED;

void cleanup_resources() {
    printf("\n[Game Engine] Commencing cleanup protocols...\n");

    // Detach and clean shared memory elements safely
    if (dungeon != NULL) {
        munmap(dungeon, sizeof(struct Dungeon));
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_unlink(dungeon_shm_name);
    }

    // Wipe global named semaphore identifiers completely
    if (lever1 != SEM_FAILED) {
        sem_close(lever1);
        sem_unlink(dungeon_lever_one);
    }
    if (lever2 != SEM_FAILED) {
        sem_close(lever2);
        sem_unlink(dungeon_lever_two);
    }
    printf("[Game Engine] All resources successfully unlinked. Goodbye.\n");
}

int main() {
    // 1. Set up pristine shared memory backing block
    shm_fd = shm_open(dungeon_shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("[Launcher] Failed to initialize shared memory segment");
        exit(1);
    }

    if (ftruncate(shm_fd, sizeof(struct Dungeon)) == -1) {
        perror("[Launcher] Failed to scale shared memory mapping size bounds");
        cleanup_resources();
        exit(1);
    }

    dungeon = (struct Dungeon*) mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("[Launcher] Failed to map memory block space");
        cleanup_resources();
        exit(1);
    }

    // Initialize memory structure values precisely
    memset(dungeon, 0, sizeof(struct Dungeon));
    dungeon->running = true;

    // 2. Initialize Named Semaphores using exact variable identities from dungeon_info.h
    sem_unlink(dungeon_lever_one);
    sem_unlink(dungeon_lever_two);

    lever1 = sem_open(dungeon_lever_one, O_CREAT, 0666, 1);
    lever2 = sem_open(dungeon_lever_two, O_CREAT, 0666, 1);

    if (lever1 == SEM_FAILED || lever2 == SEM_FAILED) {
        perror("[Launcher] Failed to initialize named dungeon semaphores");
        cleanup_resources();
        exit(1);
    }

    printf("[Game Engine] Shared tables and locking pins established successfully.\n");

    // 3. Concurrently fork and execute the sub-character binaries
    // Fork Barbarian
    pid_barbarian = fork();
    if (pid_barbarian == 0) {
        execl("./barbarian", "./barbarian", NULL);
        perror("[Launcher] Failed to spawn Barbarian binary asset");
        exit(1);
    }

    // Fork Wizard
    pid_wizard = fork();
    if (pid_wizard == 0) {
        execl("./wizard", "./wizard", NULL);
        perror("[Launcher] Failed to spawn Wizard binary asset");
        exit(1);
    }

    // Fork Rogue
    pid_rogue = fork();
    if (pid_rogue == 0) {
        execl("./rogue", "./rogue", NULL);
        perror("[Launcher] Failed to spawn Rogue binary asset");
        exit(1);
    }

    // Allow process trees a brief microsecond slice to map their handlers comfortably
    usleep(150000);

    printf("[Game Engine] Party assembled. Relinquishing core processing loop control to RunDungeon.\n");

    /* 
     * 4. Call RunDungeon matching your exact method signature requirement:
     * void RunDungeon(pid_t wizard, pid_t rogue, pid_t barbarian);
     * Note: Order matters! Wizard is parameter 1, Rogue is 2, Barbarian is 3.
     */
    RunDungeon(pid_wizard, pid_rogue, pid_barbarian);

    // 5. Post-run system teardown loops
    printf("[Game Engine] Dungeon sequence concluded. Collecting party members...\n");
    
    // Set running state false to cascade exit conditions cleanly down to all children loop states
    dungeon->running = false;

    waitpid(pid_barbarian, NULL, 0);
    waitpid(pid_wizard, NULL, 0);
    waitpid(pid_rogue, NULL, 0);

    cleanup_resources();
    return 0;
}
