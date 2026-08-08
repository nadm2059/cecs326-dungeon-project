```c
/*
 * rogue.c - CECS 326 Lab 2
 *
 * Character Process: Rogue
 *
 * This program represents the Rogue child process.
 *
 * The Rogue has two major jobs:
 *
 * 1. When DUNGEON_SIGNAL arrives:
 *      - Attempt to pick the dungeon trap lock.
 *      - Use binary search to find the correct pick angle.
 *
 * 2. When SEMAPHORE_SIGNAL arrives:
 *      - Search the treasure array.
 *      - Collect all 4 treasure characters.
 *      - Copy them into the spoils array.
 *
 * The Rogue and Barbarian communicate indirectly through:
 *
 *      - shared memory
 *      - signals
 *      - named semaphores
 */


/*
 * Provides printf() and perror().
 */
#include <stdio.h>


/*
 * Provides exit().
 */
#include <stdlib.h>


/*
 * Provides the bool data type and true/false.
 */
#include <stdbool.h>


/*
 * Provides POSIX functions such as:
 *
 *     usleep()
 *     pause()
 *     close()
 */
#include <unistd.h>


/*
 * Provides signal functionality such as:
 *
 *     sigaction()
 *     sigemptyset()
 */
#include <signal.h>


/*
 * Provides file-control constants such as:
 *
 *     O_RDWR
 */
#include <fcntl.h>


/*
 * Provides shared-memory functions such as:
 *
 *     mmap()
 *     munmap()
 *     MAP_SHARED
 */
#include <sys/mman.h>


/*
 * Provides POSIX semaphore functionality.
 */
#include <semaphore.h>


/*
 * Include the project's custom definitions.
 *
 * This file should define things such as:
 *
 *     struct Dungeon
 *     dungeon_shm_name
 *     DUNGEON_SIGNAL
 *     SEMAPHORE_SIGNAL
 *     MAX_PICK_ANGLE
 *     TIME_BETWEEN_ROGUE_TICKS
 */
#include "dungeon_info.h"


/*
 * Global pointer to the shared Dungeon structure.
 *
 * It starts as NULL because the shared memory has not
 * been mapped yet.
 *
 * After mmap() succeeds, dungeon points to the shared
 * Dungeon structure.
 */
struct Dungeon *dungeon = NULL;


/*
 * ============================================================
 * SIGNAL HANDLER
 * ============================================================
 *
 * This function handles signals sent to the Rogue process.
 *
 * The parameter 'sig' contains the number of the signal
 * that was received.
 */
void handle_signals(int sig)
{
    /*
     * Make sure that the shared-memory pointer is valid.
     *
     * If dungeon == NULL, we cannot safely access:
     *
     *     dungeon->...
     *
     * So we immediately return.
     */
    if (dungeon == NULL)
    {
        return;
    }


    /*
     * ========================================================
     * DUNGEON_SIGNAL
     * ========================================================
     *
     * Check whether the received signal is the dungeon signal.
     *
     * This signal tells the Rogue to attempt to pick the trap.
     */
    if (sig == DUNGEON_SIGNAL)
    {
        /*
         * Print a message telling the user that the Rogue
         * has encountered the trap.
         */
        printf(
            "[Rogue] Trap mechanism encountered! "
            "Commencing binary lock-picking.\n"
        );


        /*
         * Create the lower boundary of the binary-search range.
         *
         * The Rogue starts searching from angle 0.0 degrees.
         */
        float low = 0.0f;


        /*
         * Create the upper boundary of the binary-search range.
         *
         * MAX_PICK_ANGLE is defined in dungeon_info.h.
         *
         * For example, if:
         *
         *     MAX_PICK_ANGLE = 180.0
         *
         * then the Rogue initially searches:
         *
         *     0 <= angle <= 180
         */
        float high = MAX_PICK_ANGLE;


        /*
         * Variable that stores the Rogue's current guess.
         *
         * Each iteration will calculate a new midpoint.
         */
        float guess;


        /*
         * Continue attempting to pick the lock while:
         *
         * 1. The trap is still locked.
         * 2. The dungeon is still running.
         *
         * If either condition becomes false, the loop stops.
         */
        while (
            dungeon->trap.locked &&
            dungeon->running
        )
        {
            /*
             * Calculate the midpoint between low and high.
             *
             * This is the key operation of binary search.
             *
             * Example:
             *
             *     low  = 0
             *     high = 180
             *
             * Then:
             *
             *     guess = 90
             */
            guess = low + (high - low) / 2.0f;


            /*
             * Store the Rogue's current pick angle
             * in shared memory.
             *
             * Other processes can therefore see the angle
             * the Rogue is currently testing.
             */
            dungeon->rogue.pick = guess;


            /*
             * Set the trap direction to 't'.
             *
             * 't' acts as a temporary/waiting state.
             *
             * The Rogue is essentially saying:
             *
             *     "I have submitted my guess.
             *      I am waiting for the dungeon system
             *      to tell me whether I should go up,
             *      down, or whether the lock is solved."
             */
            dungeon->trap.direction = 't';


            /*
             * Wait for the dungeon system to evaluate
             * the Rogue's guess.
             *
             * The loop continues while:
             *
             *     direction == 't'
             *
             * AND:
             *
             *     trap is still locked
             *
             * AND:
             *
             *     dungeon is still running.
             */
            while (
                dungeon->trap.direction == 't' &&
                dungeon->trap.locked &&
                dungeon->running
            )
            {
                /*
                 * Sleep for the amount of time defined by:
                 *
                 *     TIME_BETWEEN_ROGUE_TICKS
                 *
                 * This prevents the Rogue from continuously
                 * checking shared memory at maximum CPU speed.
                 */
                usleep(TIME_BETWEEN_ROGUE_TICKS);
            }


            /*
             * Check whether the dungeon system said:
             *
             *     'u'
             *
             * which means the correct angle is higher.
             */
            if (dungeon->trap.direction == 'u')
            {
                /*
                 * Move the lower boundary up to the current guess.
                 *
                 * This eliminates all values below the guess.
                 *
                 * Example:
                 *
                 *     low  = 0
                 *     high = 180
                 *     guess = 90
                 *
                 * If direction == 'u':
                 *
                 *     low = 90
                 *
                 * Search becomes:
                 *
                 *     90 <= answer <= 180
                 */
                low = guess;
            }


            /*
             * Otherwise, check whether the dungeon system
             * said the correct value is lower.
             */
            else if (dungeon->trap.direction == 'd')
            {
                /*
                 * Move the upper boundary down to the current guess.
                 *
                 * This eliminates values above the guess.
                 */
                high = guess;
            }


            /*
             * Otherwise, check whether the dungeon system
             * indicated that the lock has been successfully picked.
             */
            else if (dungeon->trap.direction == '-')
            {
                /*
                 * Print the successful final angle.
                 *
                 * %.2f means:
                 *
                 *     print the floating-point number
                 *     with exactly 2 digits after the decimal.
                 */
                printf(
                    "[Rogue] Lock cleared! Final position: %.2f\n",
                    guess
                );


                /*
                 * Exit the binary-search loop.
                 */
                break;
            }
        }
    }


    /*
     * ========================================================
     * SEMAPHORE_SIGNAL
     * ========================================================
     *
     * If the signal was not DUNGEON_SIGNAL, check whether
     * it was SEMAPHORE_SIGNAL.
     *
     * This signal tells the Rogue to collect treasure.
     */
    else if (sig == SEMAPHORE_SIGNAL)
    {
        /*
         * Tell the user that the Rogue is beginning
         * the treasure-collection operation.
         */
        printf(
            "[Rogue] Party levers secured! "
            "Sweeping treasure chamber...\n"
        );


        /*
         * Keep track of how many treasure pieces
         * have been collected.
         *
         * Initially, nothing has been collected.
         */
        int collected = 0;


        /*
         * Continue collecting while:
         *
         *     collected < 4
         *
         * AND:
         *
         *     the dungeon is still running.
         *
         * Since the goal is 4 items, valid values of
         * collected are:
         *
         *     0
         *     1
         *     2
         *     3
         */
        while (
            collected < 4 &&
            dungeon->running
        )
        {
            /*
             * Read one character from the treasure array.
             *
             * collected determines which index is being examined.
             *
             * First iteration:
             *
             *     treasure[0]
             *
             * Second:
             *
             *     treasure[1]
             *
             * etc.
             */
            char item = dungeon->treasure[collected];


            /*
             * Check whether the character represents
             * a valid treasure item.
             *
             * The item must NOT be:
             *
             *     '\0'
             *     ' '
             *     '-'
             *
             * Therefore, this condition means:
             *
             *     "Only accept actual treasure characters."
             */
            if (
                item != '\0' &&
                item != ' ' &&
                item != '-'
            )
            {
                /*
                 * Copy the treasure character into the
                 * corresponding position in the spoils array.
                 *
                 * Example:
                 *
                 *     treasure[0] = 'G'
                 *
                 * becomes:
                 *
                 *     spoils[0] = 'G'
                 */
                dungeon->spoils[collected] = item;


                /*
                 * Print which treasure piece was collected.
                 *
                 * collected + 1 is used because humans normally
                 * count from 1, while arrays count from 0.
                 *
                 * Therefore:
                 *
                 * collected = 0
                 * prints piece 1.
                 *
                 * collected = 1
                 * prints piece 2.
                 */
                printf(
                    "[Rogue] Collected artifact piece %d: %c\n",
                    collected + 1,
                    item
                );


                /*
                 * Increase the number of collected items.
                 *
                 * Example:
                 *
                 *     collected = 0
                 *
                 * becomes:
                 *
                 *     collected = 1
                 */
                collected++;
            }


            /*
             * Wait 20,000 microseconds before scanning again.
             *
             * 20,000 microseconds = 20 milliseconds.
             *
             * This gives the shared-memory state time to change
             * and prevents the Rogue from consuming excessive CPU.
             */
            usleep(20000);
        }


        /*
         * This message is printed after the Rogue has either:
         *
         *     collected all 4 pieces
         *
         * OR:
         *
         *     the dungeon stopped running.
         */
        printf(
            "[Rogue] Vault completely cleared out! "
            "Signals sent to party to drop levers.\n"
        );
    }
}


/*
 * ============================================================
 * MAIN FUNCTION
 * ============================================================
 *
 * The Rogue program begins execution here.
 */
int main()
{
    /*
     * Open the existing shared-memory object.
     *
     * dungeon_shm_name:
     *     Name of the shared-memory object.
     *
     * O_RDWR:
     *     Open for reading and writing.
     *
     * 0666:
     *     Permission mode.
     *
     * Notice that O_CREAT is NOT used.
     *
     * The parent game.c program is expected to create
     * the shared-memory object first.
     */
    int shm_fd = shm_open(
        dungeon_shm_name,
        O_RDWR,
        0666
    );


    /*
     * Check whether shm_open() failed.
     *
     * A negative return value indicates failure.
     */
    if (shm_fd < 0)
    {
        /*
         * Print the reason for the failure.
         */
        perror(
            "[Rogue] Failed to open shared memory"
        );


        /*
         * Terminate the Rogue process.
         */
        exit(1);
    }


    /*
     * Map the shared-memory object into the Rogue's
     * virtual address space.
     *
     * NULL:
     *     Let the OS choose the address.
     *
     * sizeof(struct Dungeon):
     *     Number of bytes to map.
     *
     * PROT_READ | PROT_WRITE:
     *     Rogue can read and write the memory.
     *
     * MAP_SHARED:
     *     Changes are visible to other processes
     *     using the same shared-memory object.
     *
     * shm_fd:
     *     File descriptor from shm_open().
     *
     * 0:
     *     Start mapping from the beginning.
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
     * Check whether mmap() failed.
     */
    if (dungeon == MAP_FAILED)
    {
        /*
         * Print the error message.
         */
        perror(
            "[Rogue] Failed to map shared memory"
        );


        /*
         * Terminate because the Rogue cannot access
         * the Dungeon structure.
         */
        exit(1);
    }


    /*
     * Create a sigaction structure.
     *
     * This structure will tell Linux how to handle
     * incoming signals.
     */
    struct sigaction sa;


    /*
     * Set the function that should execute when
     * the registered signals arrive.
     *
     * Both signals will call:
     *
     *     handle_signals()
     */
    sa.sa_handler = handle_signals;


    /*
     * Initialize the signal mask to empty.
     *
     * No additional signals are explicitly blocked
     * while the handler is running.
     */
    sigemptyset(&sa.sa_mask);


    /*
     * Set SA_RESTART.
     *
     * This tells the operating system to restart certain
     * interrupted system calls when possible.
     */
    sa.sa_flags = SA_RESTART;


    /*
     * Register handle_signals() for DUNGEON_SIGNAL.
     *
     * When DUNGEON_SIGNAL arrives:
     *
     *     handle_signals(DUNGEON_SIGNAL)
     *
     * will execute.
     */
    sigaction(
        DUNGEON_SIGNAL,
        &sa,
        NULL
    );


    /*
     * Register handle_signals() for SEMAPHORE_SIGNAL.
     *
     * When SEMAPHORE_SIGNAL arrives:
     *
     *     handle_signals(SEMAPHORE_SIGNAL)
     *
     * will execute.
     */
    sigaction(
        SEMAPHORE_SIGNAL,
        &sa,
        NULL
    );


    /*
     * Tell the user that the Rogue has initialized
     * and is waiting for instructions.
     */
    printf(
        "[Rogue] Hidden in shadows, awaiting signals.\n"
    );


    /*
     * Continue running while the shared Dungeon structure
     * says that the game is active.
     */
    while (dungeon->running)
    {
        /*
         * Sleep until a signal arrives.
         *
         * The Rogue does not need to continuously check
         * for work.
         *
         * When a signal arrives, handle_signals() runs.
         */
        pause();
    }


    /*
     * ========================================================
     * CLEANUP
     * ========================================================
     */


    /*
     * Remove the shared-memory mapping from the Rogue's
     * address space.
     *
     * This does not destroy the shared-memory object itself.
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
     * Return 0 to indicate successful termination.
     */
    return 0;
}
```
