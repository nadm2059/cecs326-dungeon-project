
/*
 * game.c - CECS 326 Lab 2
 *
 * Master Game Launcher Orchestrator
 *
 * This program is the main/parent process for the dungeon game.
 * Its main responsibilities are:
 *
 * 1. Create shared memory.
 * 2. Initialize the shared Dungeon structure.
 * 3. Create named semaphores.
 * 4. Create child processes for:
 *      - Barbarian
 *      - Wizard
 *      - Rogue
 * 5. Replace each child process with its corresponding program
 *    using execl().
 * 6. Call RunDungeon() to coordinate the game.
 * 7. Tell all children to stop.
 * 8. Wait for all children to finish.
 * 9. Clean up shared memory and semaphores.
 */


/* Provides standard input/output functions such as printf() and perror(). */
#include <stdio.h>

/* Provides general-purpose functions such as exit(). */
#include <stdlib.h>

/* Provides POSIX functions such as fork(), execl(), usleep(), close(),
 * and other operating-system-level functions. */
#include <unistd.h>

/* Provides file-control constants and functions such as O_CREAT,
 * O_RDWR, and shm_open(). */
#include <fcntl.h>

/* Provides shared-memory functions such as mmap(), munmap(),
 * and MAP_SHARED. */
#include <sys/mman.h>

/* Provides file/stat-related definitions such as permission modes. */
#include <sys/stat.h>

/* Provides system data types such as pid_t. */
#include <sys/types.h>

/* Provides process-management functions such as waitpid(). */
#include <sys/wait.h>

/* Provides POSIX semaphore functions such as sem_open(),
 * sem_close(), and sem_unlink(). */
#include <semaphore.h>

/* Provides memory functions such as memset(). */
#include <string.h>

/* Includes the project's custom definitions.
 *
 * This file should contain things such as:
 *      struct Dungeon
 *      dungeon_shm_name
 *      dungeon_lever_one
 *      dungeon_lever_two
 *      RunDungeon()
 */
#include "dungeon_info.h"


/*
 * Stores the process ID of the Barbarian child process.
 *
 * pid_t is a data type used by Unix/Linux to represent process IDs.
 */
pid_t pid_barbarian;


/*
 * Stores the process ID of the Wizard child process.
 */
pid_t pid_wizard;


/*
 * Stores the process ID of the Rogue child process.
 */
pid_t pid_rogue;


/*
 * Stores the file descriptor returned by shm_open().
 *
 * We initialize it to -1 to indicate that shared memory
 * has not been successfully opened yet.
 */
int shm_fd = -1;


/*
 * Pointer to the shared Dungeon structure.
 *
 * The parent and child processes will use this memory region
 * to communicate with each other.
 *
 * NULL means that the pointer is not currently pointing
 * to a valid memory mapping.
 */
struct Dungeon *dungeon = NULL;


/*
 * Pointer to the first named semaphore.
 *
 * SEM_FAILED is used to indicate that sem_open() failed.
 */
sem_t *lever1 = SEM_FAILED;


/*
 * Pointer to the second named semaphore.
 *
 * SEM_FAILED indicates that sem_open() failed.
 */
sem_t *lever2 = SEM_FAILED;


/*
 * cleanup_resources()
 *
 * This function releases all resources created by the program.
 *
 * Resources being cleaned up include:
 *
 * 1. The shared-memory mapping.
 * 2. The shared-memory file descriptor.
 * 3. The named shared-memory object itself.
 * 4. The first semaphore.
 * 5. The second semaphore.
 *
 * This function is especially important because operating-system
 * resources should be released when the program finishes.
 */
void cleanup_resources()
{
    /*
     * Print a message telling the user that cleanup has started.
     *
     * \n moves the cursor to a new line.
     */
    printf("\n[Game Engine] Commencing cleanup protocols...\n");


    /*
     * Check whether dungeon points to a valid memory mapping.
     *
     * If dungeon is not NULL, the program previously mapped
     * shared memory using mmap().
     */
    if (dungeon != NULL)
    {
        /*
         * munmap() removes the shared-memory mapping from
         * this process's address space.
         *
         * dungeon:
         *     Address of the mapped memory.
         *
         * sizeof(struct Dungeon):
         *     Number of bytes that were mapped.
         */
        munmap(dungeon, sizeof(struct Dungeon));
    }


    /*
     * Check whether shm_fd contains a valid file descriptor.
     *
     * We initialized shm_fd to -1, so -1 means that
     * shm_open() has not successfully created/opened it.
     */
    if (shm_fd != -1)
    {
        /*
         * close() releases the file descriptor.
         */
        close(shm_fd);


        /*
         * shm_unlink() removes the named shared-memory object.
         *
         * dungeon_shm_name is defined in dungeon_info.h.
         *
         * Important:
         * munmap() removes the mapping from this process,
         * while shm_unlink() removes the named shared-memory
         * object from the system.
         */
        shm_unlink(dungeon_shm_name);
    }


    /*
     * Check whether the first semaphore was successfully opened.
     *
     * SEM_FAILED means sem_open() failed.
     */
    if (lever1 != SEM_FAILED)
    {
        /*
         * sem_close() closes this process's connection
         * to the named semaphore.
         */
        sem_close(lever1);


        /*
         * sem_unlink() removes the semaphore's name from
         * the system.
         *
         * dungeon_lever_one is defined in dungeon_info.h.
         */
        sem_unlink(dungeon_lever_one);
    }


    /*
     * Check whether the second semaphore was successfully opened.
     */
    if (lever2 != SEM_FAILED)
    {
        /*
         * Close the second semaphore.
         */
        sem_close(lever2);


        /*
         * Remove the second semaphore's name from the system.
         */
        sem_unlink(dungeon_lever_two);
    }


    /*
     * Tell the user that cleanup has completed.
     */
    printf("[Game Engine] All resources successfully unlinked. Goodbye.\n");
}


/*
 * main()
 *
 * This is where execution of the game launcher begins.
 */
int main()
{
    /*
     * ============================================================
     * 1. CREATE AND INITIALIZE SHARED MEMORY
     * ============================================================
     */


    /*
     * shm_open() creates or opens a POSIX shared-memory object.
     *
     * dungeon_shm_name:
     *     Name of the shared-memory object.
     *
     * O_CREAT:
     *     Create the object if it does not already exist.
     *
     * O_RDWR:
     *     Open it for both reading and writing.
     *
     * 0666:
     *     File permissions.
     *
     * The return value is stored in shm_fd.
     */
    shm_fd = shm_open(
        dungeon_shm_name,
        O_CREAT | O_RDWR,
        0666
    );


    /*
     * shm_open() returns a negative value if it fails.
     */
    if (shm_fd < 0)
    {
        /*
         * perror() prints a human-readable description
         * of the most recent system error.
         */
        perror("[Launcher] Failed to initialize shared memory segment");


        /*
         * Exit the program with status code 1.
         *
         * 0 normally means success.
         * Non-zero usually means an error occurred.
         */
        exit(1);
    }


    /*
     * ftruncate() changes the size of the shared-memory object.
     *
     * shm_fd:
     *     File descriptor returned by shm_open().
     *
     * sizeof(struct Dungeon):
     *     Number of bytes required to store one Dungeon structure.
     *
     * Without this step, the shared-memory object may have size 0,
     * which would make the mmap() operation invalid.
     */
    if (ftruncate(shm_fd, sizeof(struct Dungeon)) == -1)
    {
        /*
         * Print an error message if ftruncate() failed.
         */
        perror("[Launcher] Failed to scale shared memory mapping size bounds");


        /*
         * Attempt to release any resources that were already created.
         */
        cleanup_resources();


        /*
         * Stop the program because the shared memory could not
         * be correctly sized.
         */
        exit(1);
    }


    /*
     * mmap() maps the shared-memory object into this process's
     * virtual address space.
     *
     * NULL:
     *     Let the operating system choose the address.
     *
     * sizeof(struct Dungeon):
     *     Number of bytes to map.
     *
     * PROT_READ | PROT_WRITE:
     *     The process can read and write the memory.
     *
     * MAP_SHARED:
     *     Changes made to the mapping are shared with
     *     other processes mapping the same object.
     *
     * shm_fd:
     *     Shared-memory file descriptor.
     *
     * 0:
     *     Start mapping at the beginning of the object.
     *
     * The result is converted into a pointer to struct Dungeon.
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
     * mmap() returns MAP_FAILED when the mapping fails.
     */
    if (dungeon == MAP_FAILED)
    {
        /*
         * Print the reason mmap() failed.
         */
        perror("[Launcher] Failed to map memory block space");


        /*
         * Clean up resources created before the failure.
         */
        cleanup_resources();


        /*
         * Stop the program because the shared memory
         * could not be mapped.
         */
        exit(1);
    }


    /*
     * Clear the entire Dungeon structure.
     *
     * memset() fills a block of memory with a specified byte value.
     *
     * dungeon:
     *     Starting address.
     *
     * 0:
     *     Fill the memory with zero bytes.
     *
     * sizeof(struct Dungeon):
     *     Number of bytes to clear.
     *
     * This effectively initializes the entire structure to zero.
     */
    memset(
        dungeon,
        0,
        sizeof(struct Dungeon)
    );


    /*
     * Set the running field of the shared Dungeon structure to true.
     *
     * This tells the child processes that the game is currently running.
     *
     * Because dungeon points to shared memory, the child processes
     * can see this value.
     */
    dungeon->running = true;


    /*
     * ============================================================
     * 2. INITIALIZE NAMED SEMAPHORES
     * ============================================================
     */


    /*
     * Remove any old instance of the first named semaphore.
     *
     * This helps ensure that the game starts with a clean semaphore.
     *
     * If the semaphore does not exist, sem_unlink() simply fails,
     * which is not necessarily a problem here.
     */
    sem_unlink(dungeon_lever_one);


    /*
     * Remove any old instance of the second named semaphore.
     */
    sem_unlink(dungeon_lever_two);


    /*
     * Create/open the first named semaphore.
     *
     * dungeon_lever_one:
     *     Name of the semaphore.
     *
     * O_CREAT:
     *     Create it if it doesn't exist.
     *
     * 0666:
     *     Permission settings.
     *
     * 1:
     *     Initial semaphore value.
     *
     * A value of 1 generally means the semaphore initially
     * allows one process/thread to enter the protected section.
     */
    lever1 = sem_open(
        dungeon_lever_one,
        O_CREAT,
        0666,
        1
    );


    /*
     * Create/open the second named semaphore.
     *
     * It also starts with a value of 1.
     */
    lever2 = sem_open(
        dungeon_lever_two,
        O_CREAT,
        0666,
        1
    );


    /*
     * Check whether either semaphore failed to open.
     *
     * sem_open() returns SEM_FAILED on failure.
     */
    if (lever1 == SEM_FAILED || lever2 == SEM_FAILED)
    {
        /*
         * Print the reason for the semaphore failure.
         */
        perror("[Launcher] Failed to initialize named dungeon semaphores");


        /*
         * Clean up resources that were successfully created.
         */
        cleanup_resources();


        /*
         * Stop the program because the synchronization
         * system could not be initialized.
         */
        exit(1);
    }


    /*
     * Tell the user that the shared memory and semaphores
     * were successfully initialized.
     */
    printf(
        "[Game Engine] Shared tables and locking pins established successfully.\n"
    );


    /*
     * ============================================================
     * 3. CREATE THE THREE CHILD PROCESSES
     * ============================================================
     *
     * fork() creates a new process.
     *
     * After fork():
     *
     * Parent process:
     *     receives the child's PID as the return value.
     *
     * Child process:
     *     receives 0 as the return value.
     *
     * Therefore, code such as:
     *
     *     if (pid_barbarian == 0)
     *
     * executes only in the child.
     */


    /*
     * ------------------------------------------------------------
     * FORK BARBARIAN
     * ------------------------------------------------------------
     */


    /*
     * Create a new process for the Barbarian.
     */
    pid_barbarian = fork();


    /*
     * If fork() returns 0, this is the newly created child process.
     */
    if (pid_barbarian == 0)
    {
        /*
         * Replace the current child process with the
         * ./barbarian executable.
         *
         * execl() does NOT create another process.
         *
         * Instead, it replaces the current process's program
         * with the specified executable.
         *
         * First argument:
         *     Path to executable.
         *
         * Second argument:
         *     argv[0], normally the program name.
         *
         * NULL:
         *     Marks the end of the argument list.
         */
        execl(
            "./barbarian",
            "./barbarian",
            NULL
        );


        /*
         * If execl() succeeds, this line is NEVER reached.
         *
         * If we reach this line, execl() failed.
         */
        perror("[Launcher] Failed to spawn Barbarian binary asset");


        /*
         * Terminate the Barbarian child process.
         */
        exit(1);
    }


    /*
     * ------------------------------------------------------------
     * FORK WIZARD
     * ------------------------------------------------------------
     */


    /*
     * Create a second child process.
     */
    pid_wizard = fork();


    /*
     * If the return value is 0, we are inside the Wizard child.
     */
    if (pid_wizard == 0)
    {
        /*
         * Replace the Wizard child process with the
         * ./wizard executable.
         */
        execl(
            "./wizard",
            "./wizard",
            NULL
        );


        /*
         * This line executes only if execl() failed.
         */
        perror("[Launcher] Failed to spawn Wizard binary asset");


        /*
         * Terminate the Wizard child after the failed exec.
         */
        exit(1);
    }


    /*
     * ------------------------------------------------------------
     * FORK ROGUE
     * ------------------------------------------------------------
     */


    /*
     * Create a third child process.
     */
    pid_rogue = fork();


    /*
     * If the return value is 0, we are inside the Rogue child.
     */
    if (pid_rogue == 0)
    {
        /*
         * Replace the Rogue child process with
         * the ./rogue executable.
         */
        execl(
            "./rogue",
            "./rogue",
            NULL
        );


        /*
         * This line executes only if execl() failed.
         */
        perror("[Launcher] Failed to spawn Rogue binary asset");


        /*
         * Terminate the Rogue child process.
         */
        exit(1);
    }


    /*
     * ============================================================
     * 4. GIVE THE CHILD PROCESSES TIME TO START
     * ============================================================
     */


    /*
     * usleep() pauses the current process for a specified
     * number of microseconds.
     *
     * 150000 microseconds = 0.15 seconds = 150 milliseconds.
     *
     * The purpose here is to give the three child processes
     * a short amount of time to start and initialize their
     * own resources.
     */
    usleep(150000);


    /*
     * Print a message saying that the three character processes
     * have been created and the launcher is about to start
     * the dungeon-control logic.
     */
    printf(
        "[Game Engine] Party assembled. "
        "Relinquishing core processing loop control to RunDungeon.\n"
    );


    /*
     * ============================================================
     * 5. START THE DUNGEON CONTROL LOGIC
     * ============================================================
     *
     * According to the required function signature:
     *
     *     void RunDungeon(pid_t wizard, pid_t rogue, pid_t barbarian);
     *
     * The arguments MUST be passed in this order:
     *
     *     1. Wizard PID
     *     2. Rogue PID
     *     3. Barbarian PID
     *
     * Therefore:
     *
     *     RunDungeon(pid_wizard, pid_rogue, pid_barbarian);
     */


    /*
     * Call RunDungeon() and give it the process IDs
     * of the three child processes.
     *
     * RunDungeon() presumably coordinates the dungeon game,
     * including things such as waiting for characters,
     * handling signals, or monitoring the children.
     */
    RunDungeon(
        pid_wizard,
        pid_rogue,
        pid_barbarian
    );


    /*
     * ============================================================
     * 6. SHUT DOWN THE GAME
     * ============================================================
     */


    /*
     * Tell the user that RunDungeon() has finished and
     * the game is entering its shutdown phase.
     */
    printf(
        "[Game Engine] Dungeon sequence concluded. "
        "Collecting party members...\n"
    );


    /*
     * Change the shared running flag to false.
     *
     * The child processes can access this same shared-memory
     * structure.
     *
     * Therefore, setting:
     *
     *     dungeon->running = false;
     *
     * tells the child processes that the game is over.
     *
     * Their loops can check this value and terminate cleanly.
     */
    dungeon->running = false;


    /*
     * ============================================================
     * 7. WAIT FOR THE CHILD PROCESSES
     * ============================================================
     */


    /*
     * waitpid() waits for a specific child process to finish.
     *
     * pid_barbarian:
     *     PID of the Barbarian process.
     *
     * NULL:
     *     We do not need the child's exit status.
     *
     * 0:
     *     Wait normally until the child exits.
     */
    waitpid(
        pid_barbarian,
        NULL,
        0
    );


    /*
     * Wait for the Wizard process to finish.
     */
    waitpid(
        pid_wizard,
        NULL,
        0
    );


    /*
     * Wait for the Rogue process to finish.
     */
    waitpid(
        pid_rogue,
        NULL,
        0
    );


    /*
     * ============================================================
     * 8. CLEAN UP ALL RESOURCES
     * ============================================================
     */


    /*
     * Release:
     *
     * - shared-memory mapping
     * - shared-memory object
     * - file descriptor
     * - first semaphore
     * - second semaphore
     */
    cleanup_resources();


    /*
     * Return 0 from main().
     *
     * A return value of 0 conventionally means that the program
     * completed successfully.
     */
    return 0;
}
```
