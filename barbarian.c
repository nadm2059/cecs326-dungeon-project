
/*
 * barbarian.c - CECS 326 Lab 2
 *
 * Character Process: Barbarian
 *
 * This program represents the Barbarian child process.
 *
 * Its main responsibilities are:
 *
 * 1. Open the shared-memory Dungeon structure created by game.c.
 * 2. Map that shared memory into its address space.
 * 3. Install signal handlers.
 * 4. Wait for signals from the game system.
 * 5. When DUNGEON_SIGNAL is received:
 *      - Copy the enemy's health into the Barbarian's attack field.
 * 6. When SEMAPHORE_SIGNAL is received:
 *      - Open Lever 1.
 *      - Lock the semaphore.
 *      - Wait for the Rogue to collect all treasure.
 *      - Release the semaphore.
 * 7. Continue running until dungeon->running becomes false.
 */


/*
 * Provides standard input/output functions such as:
 * printf()
 * perror()
 */
#include <stdio.h>


/*
 * Provides functions such as:
 * exit()
 */
#include <stdlib.h>


/*
 * Provides the bool data type and true/false values.
 *
 * In this file, bool is mainly used indirectly through
 * the Dungeon structure's running field.
 */
#include <stdbool.h>


/*
 * Provides POSIX functions such as:
 * pause()
 * usleep()
 * close()
 */
#include <unistd.h>


/*
 * Provides signal-related functionality such as:
 * signal numbers
 * sigaction()
 * sigemptyset()
 * pause()
 */
#include <signal.h>


/*
 * Provides file-control constants such as:
 * O_RDWR
 */
#include <fcntl.h>


/*
 * Provides memory-mapping functions such as:
 * mmap()
 * munmap()
 * MAP_SHARED
 */
#include <sys/mman.h>


/*
 * Provides POSIX semaphore functionality such as:
 * sem_open()
 * sem_wait()
 * sem_post()
 * sem_close()
 */
#include <semaphore.h>


/*
 * Include the project's custom header file.
 *
 * This header should define:
 *
 * struct Dungeon
 * dungeon_shm_name
 * dungeon_lever_one
 * DUNGEON_SIGNAL
 * SEMAPHORE_SIGNAL
 * and other game-related definitions.
 */
#include "dungeon_info.h"


/*
 * Global pointer to the shared Dungeon structure.
 *
 * Initially it is NULL because the shared-memory region
 * has not been mapped yet.
 *
 * Once mmap() succeeds, dungeon will point to the
 * shared Dungeon structure.
 */
struct Dungeon *dungeon = NULL;


/*
 * ============================================================
 * SIGNAL HANDLER
 * ============================================================
 *
 * This function is called automatically by the operating system
 * whenever the Barbarian receives one of the signals that was
 * registered with sigaction().
 *
 * The parameter 'sig' contains the number of the signal received.
 */
void handle_signals(int sig)
{
    /*
     * Check whether the shared-memory pointer is NULL.
     *
     * If it is NULL, we cannot safely access dungeon->...
     *
     * return immediately if there is no valid Dungeon structure.
     */
    if (dungeon == NULL)
    {
        return;
    }


    /*
     * ========================================================
     * HANDLE DUNGEON_SIGNAL
     * ========================================================
     *
     * Check whether the received signal is DUNGEON_SIGNAL.
     */
    if (sig == DUNGEON_SIGNAL)
    {
        /*
         * Print a message telling us that the Barbarian
         * received the battle signal.
         */
        printf(
            "[Barbarian] Battle signal received! "
            "Attacking enemy health point.\n"
        );


        /*
         * Copy the enemy's current health into the
         * Barbarian's attack field.
         *
         * Conceptually:
         *
         *     Barbarian.attack = Enemy.health
         *
         * Because dungeon is shared memory, this value
         * can be seen by the other processes as well.
         *
         * For example, if:
         *
         *     dungeon->enemy.health = 50;
         *
         * then this statement makes:
         *
         *     dungeon->barbarian.attack = 50;
         */
        dungeon->barbarian.attack = dungeon->enemy.health;
    }


    /*
     * ========================================================
     * HANDLE SEMAPHORE_SIGNAL
     * ========================================================
     *
     * If the signal was not DUNGEON_SIGNAL, check whether
     * it was SEMAPHORE_SIGNAL.
     */
    else if (sig == SEMAPHORE_SIGNAL)
    {
        /*
         * Tell the user that the Barbarian received the
         * semaphore-related signal.
         */
        printf(
            "[Barbarian] Semaphore signal caught! "
            "Securing Lever 1.\n"
        );


        /*
         * Create a local pointer for Lever 1.
         *
         * sem_open() connects this process to an existing
         * named POSIX semaphore.
         *
         * dungeon_lever_one is the semaphore's name and
         * should be defined in dungeon_info.h.
         *
         * The second argument is 0.
         *
         * 0 means:
         *
         *     "Open an existing semaphore.
         *      Do not create a new one."
         */
        sem_t *lever1 = sem_open(
            dungeon_lever_one,
            0
        );


        /*
         * Check whether sem_open() failed.
         *
         * sem_open() returns SEM_FAILED when it fails.
         */
        if (lever1 == SEM_FAILED)
        {
            /*
             * Print the error generated by sem_open().
             */
            perror(
                "[Barbarian] Failed to open lever 1 semaphore"
            );


            /*
             * Stop handling this signal.
             *
             * This does NOT terminate the Barbarian program.
             * It only returns from handle_signals().
             */
            return;
        }


        /*
         * Wait for the semaphore.
         *
         * sem_wait() attempts to decrement the semaphore's
         * value.
         *
         * If the semaphore value is greater than 0:
         *
         *     the Barbarian obtains the semaphore
         *     and continues.
         *
         * If the semaphore value is 0:
         *
         *     the Barbarian waits until another process
         *     releases it using sem_post().
         *
         * This is what provides synchronization.
         */
        sem_wait(lever1);


        /*
         * Print a message confirming that the Barbarian
         * successfully acquired Lever 1.
         */
        printf(
            "[Barbarian] Lever 1 successfully held down.\n"
        );


        /*
         * ====================================================
         * WAIT FOR THE ROGUE TO FINISH COLLECTING TREASURE
         * ====================================================
         *
         * The spoils array has size 4.
         *
         * Therefore its indexes are:
         *
         *     spoils[0]
         *     spoils[1]
         *     spoils[2]
         *     spoils[3]
         *
         * Index 3 represents the fourth and final character.
         *
         * The loop waits until spoils[3] is no longer '\0'.
         *
         * '\0' means that the character position is currently
         * empty/uninitialized.
         */
        while (
            dungeon != NULL &&
            dungeon->running &&
            dungeon->spoils[3] == '\0'
        )
        {
            /*
             * Sleep for 10,000 microseconds.
             *
             * 10,000 microseconds = 10 milliseconds.
             *
             * This prevents the Barbarian from continuously
             * checking the memory as fast as possible.
             *
             * Without the sleep, this would be a busy-wait loop
             * consuming unnecessary CPU time.
             */
            usleep(10000);
        }


        /*
         * Release Lever 1.
         *
         * sem_post() increments the semaphore's value and
         * allows another process waiting for the semaphore
         * to acquire it.
         */
        sem_post(lever1);


        /*
         * Close this process's connection to the semaphore.
         *
         * This does not necessarily destroy the named semaphore.
         *
         * It simply closes this process's semaphore handle.
         */
        sem_close(lever1);


        /*
         * Tell the user that Lever 1 has been released.
         */
        printf(
            "[Barbarian] Lever 1 safely released.\n"
        );
    }
}


/*
 * ============================================================
 * MAIN FUNCTION
 * ============================================================
 *
 * Execution of the Barbarian program begins here.
 */
int main()
{
    /*
     * ========================================================
     * OPEN SHARED MEMORY
     * ========================================================
     */


    /*
     * Create a local integer to store the shared-memory
     * file descriptor.
     *
     * shm_open() returns a file descriptor that can later
     * be passed to mmap().
     *
     * dungeon_shm_name:
     *     Name of the shared-memory object.
     *
     * O_RDWR:
     *     Open it for reading and writing.
     *
     * 0666:
     *     Permission value.
     *
     * Notice that O_CREAT is NOT used here.
     *
     * That means the Barbarian expects game.c to have
     * already created the shared-memory object.
     */
    int shm_fd = shm_open(
        dungeon_shm_name,
        O_RDWR,
        0666
    );


    /*
     * Check whether shm_open() failed.
     *
     * A negative file descriptor indicates an error.
     */
    if (shm_fd < 0)
    {
        /*
         * Print a description of the error.
         */
        perror(
            "[Barbarian] Failed to open shared memory"
        );


        /*
         * Terminate the Barbarian process.
         *
         * There is no point continuing because the Barbarian
         * cannot access the shared Dungeon structure.
         */
        exit(1);
    }


    /*
     * ========================================================
     * MAP SHARED MEMORY
     * ========================================================
     */


    /*
     * Map the shared-memory object into the Barbarian's
     * virtual address space.
     *
     * NULL:
     *     Let the operating system choose the address.
     *
     * sizeof(struct Dungeon):
     *     Number of bytes to map.
     *
     * PROT_READ | PROT_WRITE:
     *     The Barbarian can read and modify the memory.
     *
     * MAP_SHARED:
     *     Changes are visible to other processes mapping
     *     the same shared-memory object.
     *
     * shm_fd:
     *     Shared-memory file descriptor.
     *
     * 0:
     *     Start at the beginning of the shared-memory object.
     *
     * The result is cast to:
     *
     *     struct Dungeon *
     *
     * so that we can use:
     *
     *     dungeon->running
     *     dungeon->enemy.health
     *     dungeon->barbarian.attack
     *     dungeon->spoils[3]
     */
    dungeon = (struct Dungeon *)mmap(
        NULL,
        sizeof(struct Dungeon),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );


    /*
     * mmap() returns MAP_FAILED if the mapping fails.
     */
    if (dungeon == MAP_FAILED)
    {
        /*
         * Print the reason mmap() failed.
         */
        perror(
            "[Barbarian] Failed to map shared memory"
        );


        /*
         * Terminate the Barbarian process.
         */
        exit(1);
    }


    /*
     * ========================================================
     * CONFIGURE SIGNAL HANDLING
     * ========================================================
     */


    /*
     * Declare a sigaction structure.
     *
     * This structure tells the operating system
     * how to handle a particular signal.
     */
    struct sigaction sa;


    /*
     * Set the signal handler function.
     *
     * Whenever one of the registered signals arrives,
     * the operating system will call:
     *
     *     handle_signals(signal_number)
     */
    sa.sa_handler = handle_signals;


    /*
     * Clear the signal mask.
     *
     * A signal mask specifies which signals should temporarily
     * be blocked while the signal handler is executing.
     *
     * sigemptyset() creates an empty mask, meaning no additional
     * signals are explicitly added to the mask.
     */
    sigemptyset(&sa.sa_mask);


    /*
     * Set the signal-action flags.
     *
     * SA_RESTART tells the operating system to automatically
     * restart certain interrupted system calls when possible.
     *
     * This can make functions such as pause(), read(), etc.
     * behave more predictably when signals arrive.
     */
    sa.sa_flags = SA_RESTART;


    /*
     * Register handle_signals() for DUNGEON_SIGNAL.
     *
     * When this signal arrives, the operating system calls:
     *
     *     handle_signals(DUNGEON_SIGNAL);
     */
    sigaction(
        DUNGEON_SIGNAL,
        &sa,
        NULL
    );


    /*
     * Register the same signal handler for SEMAPHORE_SIGNAL.
     *
     * When this signal arrives, the operating system calls:
     *
     *     handle_signals(SEMAPHORE_SIGNAL);
     */
    sigaction(
        SEMAPHORE_SIGNAL,
        &sa,
        NULL
    );


    /*
     * Print a message showing that the Barbarian is ready.
     */
    printf(
        "[Barbarian] Standing by in the dungeon.\n"
    );


    /*
     * ========================================================
     * MAIN WAITING LOOP
     * ========================================================
     */


    /*
     * Continue running while the shared Dungeon's running
     * variable is true.
     *
     * Because dungeon points to shared memory, another process
     * such as game.c can change:
     *
     *     dungeon->running = false;
     *
     * When that happens, this loop eventually ends.
     */
    while (dungeon->running)
    {
        /*
         * pause() puts the Barbarian process to sleep until
         * a signal is received.
         *
         * This is much more efficient than continuously checking
         * for signals in a busy loop.
         *
         * Example:
         *
         *     Barbarian is sleeping...
         *             |
         *             |
         *       DUNGEON_SIGNAL
         *             |
         *             v
         *     handle_signals() runs
         *             |
         *             v
         *       Barbarian sleeps again
         */
        pause();
    }


    /*
     * ========================================================
     * CLEANUP
     * ========================================================
     */


    /*
     * Remove the shared-memory mapping from the Barbarian's
     * address space.
     *
     * This does NOT destroy the shared-memory object itself.
     * The parent game.c process is responsible for eventually
     * unlinking the shared-memory object.
     */
    munmap(
        dungeon,
        sizeof(struct Dungeon)
    );


    /*
     * Close the shared-memory file descriptor.
     */
    close(shm_fd);


    /*
     * Return 0 to indicate that the Barbarian process
     * terminated successfully.
     */
    return 0;
}
```
