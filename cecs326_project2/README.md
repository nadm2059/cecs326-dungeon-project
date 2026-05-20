# IPC Dungeon Crawler Simulator

A concurrent, multiplayer terminal-based game simulation written in C that demonstrates advanced UNIX Inter-Process Communication (IPC). The system utilizes a master orchestration process to manage a party of distinct worker processes—a Barbarian, a Wizard, and a Rogue—synchronized via POSIX shared memory, custom signal handlers, and named semaphores to complete procedural challenges and secure dungeon treasure.

---

## Architecture Overview

The system architecture consists of a parent process (`game.c`) that establishes a shared memory table before branching out into three isolated worker processes through concurrent `fork()` and `execv()` calls.

* 
**Game Engine (`game.c`):** Initializes resources, manages lifecycle control, maps shared tables, and offloads core phase mechanics to a compiled game engine driver (`dungeon.o`).


* 
**Barbarian (`barbarian.c`):** Reacts instantly to combat triggers by matching volatile integers in shared memory to incapacitate targets.


* 
**Wizard (`wizard.c`):** Decodes shifting cryptographic strings using a custom sliding Caesar Cipher algorithm to bypass magical barriers.


* 
**Rogue (`rogue.c`):** Executes a high-frequency binary search algorithm to match dynamic floating-point values and disarm mechanical traps.



---

## File Structure

```text
├── Makefile                # Build automation script with decoupled linking steps
├── dungeon_info.h          # Global struct definitions for shared memory blocks
├── dungeon_settings.h      # Game tuning variables, buffers, signals, and thresholds
├── dungeon.o               # Pre-compiled core game library provided by the instructor
├── game.c                  # Master orchestrator and process manager
├── barbarian.c             # Barbarian child process source code
├── wizard.c                # Wizard child process source code
└── rogue.c                 # Rogue child process source code

```

---

## Configuration & IPC Elements

The project uses the parameters defined in `dungeon_info.h` and `dungeon_settings.h` to establish system boundaries:

### Shared Memory Allocation

A single shared memory block is created under the system name `"/DungeonMem"`. It overlays a structural format (`struct Dungeon`) across virtual memory to keep the state synchronized across all forks:

* 
`dungeon->running`: Volatile boolean checking active runtime states.


* 
`dungeon->enemy.health` & `dungeon->barbarian.attack`: Active data synchronization registers for combat.


* 
`dungeon->barrier.spell` & `dungeon->wizard.spell`: String buffers utilized to resolve ciphers.


* 
`dungeon->trap.direction` & `dungeon->rogue.pick`: Real-time feedback channels for trap tracking.



### Signal Routing Mapping

* 
**`DUNGEON_SIGNAL` (`SIGUSR1`):** Interrupt vector used to alert children that an encounter round has loaded and requires a response.


* 
**`SEMAPHORE_SIGNAL` (`SIGUSR2`):** Triggers the endgame synchronization sequence.



### Coordination Vault Semaphores

Two POSIX named binary semaphores are initialized to regulate access to the endgame rewards:

* 
`"/LeverOne"`: Assigned to the Barbarian to lock down during vault entry.


* 
`"/LeverTwo"`: Assigned to the Wizard to lock down during vault entry.



---

## Build and Run Instructions

### Prerequisites

A Linux environment (such as Ubuntu or Fedora) with `gcc`, `make`, and standard POSIX development headers installed.

### Compilation

Compile the project components and bind them safely against the pre-compiled library by running:

```bash
make clean
make

```

### Execution

Launch the primary game manager binary:

```bash
./game

```

To exit or run clean-up diagnostics after runtime completion, hit `Ctrl+C` to invoke the tracking unlinks inside the engine signal interceptors.

---

```
