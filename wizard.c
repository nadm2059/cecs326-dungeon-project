```c
/*
 * wizard.c - CECS 326 Lab 2
 *
 * Character Process: Wizard
 *
 * This program represents the Wizard child process.
 *
 * The Wizard has two major responsibilities:
 *
 * 1. When DUNGEON_SIGNAL is received:
 *      - Read the encrypted spell from the shared Dungeon.
 *      - Determine the Caesar cipher shift.
 *      - Decode the spell.
 *      - Store the decoded spell in the Wizard's spell field.
 *
 * 2. When SEMAPHORE_SIGNAL is received:
 *      - Open Lever 2.
 *      - Acquire the semaphore.
 *      - Wait for the Rogue to finish collecting treasure.
 *      - Release Lever 2.
 */


/*
 * Provides standard input/output functions.
 *
 * Used for:
 *     printf()
 *     perror()
 */
#include <stdio.h>


/*
 * Provides general-purpose functions.
 *
 * Used for:
 *     exit()
 */
#include <stdlib.h>


/*
 * Provides the Boolean data type:
 *
 *     bool
 *     true
 *     false
 */
#include <stdbool.h>


/*
 * Provides POSIX functions such as:
 *
 *     pause()
 *     usleep()
 *     close()
 */
#include <unistd.h>


/*
 * Provides signal-related functionality.
 *
 * Used for:
 *
 *     sigaction()
 *     sigemptyset()
 */
#include <signal.h>


/*
 * Provides file-control constants.
 *
 * Used for:
 *
 *     O_RDWR
 */
#include <fcntl.h>


/*
 * Provides memory-mapping functionality.
 *
 * Used for:
 *
 *     mmap()
 *     munmap()
 *     MAP_SHARED
 *     PROT_READ
 *     PROT_WRITE
 */
#include <sys/mman.h>


/*
 * Provides POSIX semaphore functionality.
 *
 * Used for:
 *
 *     sem_open()
 *     sem_wait()
 *     sem_post()
 *     sem_close()
 */
#include <semaphore.h>


/*
 * Provides character-classification functions.
 *
 * Used for:
 *
 *     isalpha()
 *     isupper()
 */
#include <ctype.h>


/*
 * Provides string-manipulation functions.
 *
 * Used for:
 *
 *     strncpy()
 */
#include <string.h>


/*
 * Include the project's custom header file.
 *
 * This file should contain definitions for things such as:
 *
 *     struct Dungeon
 *     dungeon_shm_name
 *     dungeon_lever_two
 *     DUNGEON_SIGNAL
 *     SEMAPHORE_SIGNAL
 *     SPELL_BUFFER_SIZE
 */
#include "dungeon_info.h"


/*
 * Global pointer to the shared Dungeon structure.
 *
 * Initially it is NULL because the shared memory has not
 * been mapped yet.
 *
 * After mmap() succeeds, this pointer will point to the
 * shared Dungeon structure.
 */
struct Dungeon *dungeon = NULL;


/*
 * ============================================================
 * CAESAR CIPHER DECODER
 * ============================================================
 *
 * decode_caesar()
 *
 * This function takes an encrypted string and decrypts it
 * using a Caesar cipher.
 *
 * Parameters:
 *
 *     cipher:
 *         Pointer to the encrypted string.
 *
 *     output:
 *         Pointer to the character array where the decrypted
 *         string will be stored.
 *
 * Example:
 *
 *     cipher = "3KHOOR"
 *
 * The first character '3' represents the shift.
 *
 * The remaining characters:
 *
 *     KHOOR
 *
 * are decoded using a shift of 3:
 *
 *     K -> H
 *     H -> E
 *     O -> L
 *     O -> L
 *     R -> O
 *
 * Result:
 *
 *     HELLO
 */
void decode_caesar(const char *cipher, char *output)
{
    /*
     * Check whether the encrypted string is empty.
     *
     * cipher[0] is the first character.
     *
     * '\0' means the end of a C string.
     */
    if (cipher[0] == '\0')
    {
        /*
         * If the input is empty, make the output an empty
         * C string as well.
         */
        output[0] = '\0';


        /*
         * Stop the function immediately.
         */
        return;
    }


    /*
     * Use the first character of the encrypted string
     * as the Caesar cipher shift value.
     *
     * For example, if:
     *
     *     cipher[0] = 3
     *
     * then:
     *
     *     shift = 3
     *
     * IMPORTANT:
     *
     * This code is using the NUMERIC VALUE of cipher[0],
     * not the digit character value.
     *
     * If cipher[0] is the character '3', its ASCII value
     * is 51, not integer 3.
     *
     * Therefore the exact encoding expected by the lab
     * matters here.
     */
    int shift = cipher[0];


    /*
     * Start reading the encrypted message at index 1.
     *
     * Index 0 contains the shift/key.
     *
     * Therefore we do NOT decode cipher[0].
     */
    int i = 1;


    /*
     * Keeps track of where the decoded character should
     * be written in the output array.
     *
     * It starts at 0 because output[0] should contain
     * the first decoded character.
     */
    int out_idx = 0;


    /*
     * Continue processing characters until the null
     * terminator is reached.
     *
     * cipher[i] != '\0' means:
     *
     *     "There are still characters left to decode."
     */
    while (cipher[i] != '\0')
    {
        /*
         * Copy the current encrypted character into c.
         *
         * This makes the code easier to read.
         */
        char c = cipher[i];


        /*
         * Check whether c is an alphabetic character.
         *
         * isalpha() returns true for letters such as:
         *
         *     A-Z
         *     a-z
         *
         * The cast to unsigned char is used because the
         * character-classification functions expect either
         * EOF or an unsigned-char value.
         */
        if (isalpha((unsigned char)c))
        {
            /*
             * Check whether the character is uppercase.
             */
            if (isupper((unsigned char)c))
            {
                /*
                 * Decode an uppercase letter.
                 *
                 * The Caesar cipher calculation is:
                 *
                 *     ((c - 'A' - shift) % 26 + 26) % 26 + 'A'
                 *
                 * Let's break it down.
                 *
                 * c - 'A'
                 *
                 * converts:
                 *
                 *     A -> 0
                 *     B -> 1
                 *     C -> 2
                 *     ...
                 *     Z -> 25
                 *
                 * Then:
                 *
                 *     - shift
                 *
                 * moves the character backward.
                 *
                 * The modulo 26 operation wraps around
                 * the alphabet.
                 */
                output[out_idx] =
                    ((c - 'A' - shift) % 26 + 26) % 26 + 'A';
            }


            /*
             * If the character is alphabetic but not uppercase,
             * it is treated as lowercase.
             */
            else
            {
                /*
                 * Decode a lowercase letter.
                 *
                 * The same logic is used as uppercase,
                 * except the alphabet begins at 'a'.
                 *
                 * Therefore:
                 *
                 *     a -> 0
                 *     b -> 1
                 *     ...
                 *     z -> 25
                 */
                output[out_idx] =
                    ((c - 'a' - shift) % 26 + 26) % 26 + 'a';
            }
        }


        /*
         * If the character is NOT alphabetic:
         *
         *     space
         *     punctuation
         *     number
         *     symbol
         *
         * then it is copied directly into the output.
         */
        else
        {
            /*
             * Preserve the original character.
             *
             * For example:
             *
             *     ' ' stays ' '
             *     '!' stays '!'
             *     '?' stays '?'
             *     '1' stays '1'
             */
            output[out_idx] = c;
        }


        /*
         * Move to the next character in the encrypted string.
         *
         * Example:
         *
         *     i = 1
         *
         * becomes:
         *
         *     i = 2
         */
        i++;


        /*
         * Move to the next position in the output array.
         *
         * Example:
         *
         *     out_idx = 0
         *
         * becomes:
         *
         *     out_idx = 1
         */
        out_idx++;
    }


    /*
     * Add the null terminator to the decoded output string.
     *
     * '\0' tells C where the string ends.
     *
     * Without this, printf("%s", output) would not know
     * where the string ends and could read beyond the
     * valid output buffer.
     */
    output[out_idx] = '\0';
}


/*
 * ============================================================
 * SIGNAL HANDLER
 * ============================================================
 *
 * This function is automatically called when the Wizard
 * receives one of the registered signals.
 */
void handle_signals(int sig)
{
    /*
     * Check whether the shared-memory pointer is valid.
     *
     * If it is NULL, we cannot safely access dungeon.
     */
    if (dungeon == NULL)
    {
        /*
         * Stop handling the signal.
         */
        return;
    }


    /*
     * ========================================================
     * DUNGEON_SIGNAL
     * ========================================================
     *
     * This signal tells the Wizard to decipher the
     * magical barrier spell.
     */
    if (sig == DUNGEON_SIGNAL)
    {
        /*
         * Tell the user that the Wizard received the
         * barrier/decoding signal.
         */
        printf(
            "[Wizard] Magic barrier field detected. "
            "Deciphering glyphs...\n"
        );


        /*
         * Create a local character buffer for the decoded spell.
         *
         * SPELL_BUFFER_SIZE + 1 provides space for the
         * terminating '\0'.
         *
         * {0} initializes every element of the array to zero.
         */
        char decoded[SPELL_BUFFER_SIZE + 1] = {0};


        /*
         * Decode the encrypted spell.
         *
         * The encrypted spell is stored in:
         *
         *     dungeon->barrier.spell
         *
         * The decoded result is placed into:
         *
         *     decoded
         */
        decode_caesar(
            dungeon->barrier.spell,
            decoded
        );


        /*
         * Copy the decoded spell into the Wizard's
         * shared-memory spell field.
         *
         * strncpy() copies at most:
         *
         *     sizeof(dungeon->wizard.spell) - 1
         *
         * characters.
         *
         * Leaving one byte available allows space for
         * the null terminator.
         */
        strncpy(
            dungeon->wizard.spell,
            decoded,
            sizeof(dungeon->wizard.spell) - 1
        );


        /*
         * Explicitly place a null terminator at the final
         * allowed position.
         *
         * This guarantees that dungeon->wizard.spell
         * is a valid C string even if strncpy() had to
         * truncate the decoded message.
         */
        dungeon->wizard.spell[
            sizeof(dungeon->wizard.spell) - 1
        ] = '\0';


        /*
         * Print the decoded counter-spell.
         *
         * %s prints a C string.
         *
         * dungeon->wizard.spell is the decoded result
         * stored in shared memory.
         */
        printf(
            "[Wizard] Counter-spell focused: %s\n",
            dungeon->wizard.spell
        );
    }


    /*
     * ========================================================
     * SEMAPHORE_SIGNAL
     * ========================================================
     *
     * If the signal was not DUNGEON_SIGNAL, check whether
     * it is the semaphore signal.
     *
     * This tells the Wizard to secure Lever 2.
     */
    else if (sig == SEMAPHORE_SIGNAL)
    {
        /*
         * Tell the user that the Wizard received the
         * semaphore signal.
         */
        printf(
            "[Wizard] Semaphore signal caught! "
            "Securing Lever 2.\n"
        );


        /*
         * Create a local semaphore pointer.
         *
         * sem_open() connects the Wizard to the named
         * Lever 2 semaphore.
         *
         * dungeon_lever_two is the name defined in
         * dungeon_info.h.
         *
         * 0 means:
         *
         *     Open an existing semaphore.
         *
         * The Wizard does NOT create a new semaphore here.
         */
        sem_t *lever2 = sem_open(
            dungeon_lever_two,
            0
        );


        /*
         * Check whether sem_open() failed.
         *
         * SEM_FAILED indicates failure.
         */
        if (lever2 == SEM_FAILED)
        {
            /*
             * Print the reason for the semaphore failure.
             */
            perror(
                "[Wizard] Failed to open lever 2 semaphore"
            );


            /*
             * Return from the signal handler.
             *
             * This does not terminate the Wizard program.
             */
            return;
        }


        /*
         * Wait for permission to acquire Lever 2.
         *
         * If the semaphore's value is available, the Wizard
         * acquires it.
         *
         * If another process currently holds it, the Wizard
         * waits until it becomes available.
         */
        sem_wait(lever2);


        /*
         * Tell the user that the Wizard successfully
         * acquired Lever 2.
         */
        printf(
            "[Wizard] Lever 2 successfully held down.\n"
        );


        /*
         * Wait until the Rogue has populated the fourth
         * position in the spoils array.
         *
         * spoils[3] is the fourth element because array
         * indexes start at 0.
         *
         * The loop continues while:
         *
         *     dungeon != NULL
         *
         * AND:
         *
         *     dungeon->running == true
         *
         * AND:
         *
         *     dungeon->spoils[3] == '\0'
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
             * This prevents the Wizard from constantly
             * polling the shared memory at full CPU speed.
             */
            usleep(10000);
        }


        /*
         * Release Lever 2.
         *
         * sem_post() increments the semaphore and makes
         * it available again.
         */
        sem_post(lever2);


        /*
         * Close this process's connection to the semaphore.
         *
         * This does not necessarily destroy the named semaphore.
         */
        sem_close(lever2);


        /*
         * Tell the user that Lever 2 has been released.
         */
        printf(
            "[Wizard] Lever 2 safely released.\n"
        );
    }
}


/*
 * ============================================================
 * MAIN FUNCTION
 * ============================================================
 *
 * Program execution begins here.
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
     *     Open for both reading and writing.
     *
     * 0666:
     *     Permission setting.
     *
     * O_CREAT is NOT used because game.c should already
     * have created the shared-memory object.
     */
    int shm_fd = shm_open(
        dungeon_shm_name,
        O_RDWR,
        0666
    );


    /*
     * Check whether shm_open() failed.
     *
     * A negative value indicates failure.
     */
    if (shm_fd < 0)
    {
        /*
         * Print the reason for the failure.
         */
        perror(
            "[Wizard] Failed to open shared memory"
        );


        /*
         * Terminate the Wizard process.
         */
        exit(1);
    }


    /*
     * Map the shared-memory object into the Wizard's
     * address space.
     *
     * NULL:
     *     Let the operating system choose the address.
     *
     * sizeof(struct Dungeon):
     *     Number of bytes to map.
     *
     * PROT_READ | PROT_WRITE:
     *     Allow the Wizard to read and write the memory.
     *
     * MAP_SHARED:
     *     Changes are shared with other processes.
     *
     * shm_fd:
     *     File descriptor returned by shm_open().
     *
     * 0:
     *     Start at the beginning of the shared-memory object.
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
         * Print the reason for the failure.
         */
        perror(
            "[Wizard] Failed to map shared memory"
        );


        /*
         * Terminate the Wizard because it cannot access
         * the shared Dungeon structure.
         */
        exit(1);
    }


    /*
     * Create a sigaction structure.
     *
     * This structure tells the operating system how to
     * handle signals.
     */
    struct sigaction sa;


    /*
     * Set the signal handler function.
     *
     * Whenever one of the registered signals arrives,
     * Linux calls:
     *
     *     handle_signals(signal_number)
     */
    sa.sa_handler = handle_signals;


    /*
     * Initialize the signal mask.
     *
     * An empty mask means no additional signals are
     * explicitly blocked while the handler executes.
     */
    sigemptyset(&sa.sa_mask);


    /*
     * Set the SA_RESTART flag.
     *
     * This asks the OS to restart certain interrupted
     * system calls when possible.
     */
    sa.sa_flags = SA_RESTART;


    /*
     * Register handle_signals() for DUNGEON_SIGNAL.
     *
     * When the Wizard receives DUNGEON_SIGNAL:
     *
     *     handle_signals(DUNGEON_SIGNAL)
     *
     * is executed.
     */
    sigaction(
        DUNGEON_SIGNAL,
        &sa,
        NULL
    );


    /*
     * Register handle_signals() for SEMAPHORE_SIGNAL.
     *
     * When the Wizard receives SEMAPHORE_SIGNAL:
     *
     *     handle_signals(SEMAPHORE_SIGNAL)
     *
     * is executed.
     */
    sigaction(
        SEMAPHORE_SIGNAL,
        &sa,
        NULL
    );


    /*
     * Tell the user that the Wizard has successfully
     * initialized.
     */
    printf(
        "[Wizard] Magical sensors calibrated.\n"
    );


    /*
     * Keep the Wizard alive while the shared Dungeon's
     * running flag is true.
     */
    while (dungeon->running)
    {
        /*
         * Put the Wizard to sleep until a signal arrives.
         *
         * When a signal arrives, handle_signals()
         * will execute.
         */
        pause();
    }


    /*
     * ========================================================
     * CLEANUP
     * ========================================================
     */


    /*
     * Remove the shared-memory mapping from this process.
     *
     * This does not remove the shared-memory object itself.
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
