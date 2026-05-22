
# Multi-Process Dungeon Crawler Simulator

A robust, concurrent terminal-based simulation application written in C that showcases advanced principles of UNIX Systems Programming and Inter-Process Communication (IPC). The system demonstrates high-performance parent-child orchestration, managing a dynamic party of completely autonomous worker processes (a Barbarian, a Wizard, and a Rogue). Synchronous operations are achieved via POSIX shared memory segments, real-time POSIX signal handling vectors, and counting semaphores to coordinate microsecond-accurate vault extraction phases.

---

## 🏗️ Architectural Blueprint & Process Roles

The application framework builds a distributed processing environment by allocating isolated kernel-level resource structures before executing concurrent child forks. Instead of threading within a single address space, true multi-processing via `fork()` ensures memory protection boundaries between game characters, which interface exclusively over explicit communication channels.

```text
               +----------------------------------------+
               |          game.c (Supervisor)           |
               +----------------------------------------+
                     /             |              \
                    /              |               \
        fork() & execv()    fork() & execv()   fork() & execv()
                  /                |                 \
                 v                 v                  v
        +---------------+  +---------------+  +---------------+
        |  barbarian.c  |  |   wizard.c    |  |    rogue.c    |
        +---------------+  +---------------+  +---------------+
                 \                 |                  /
                  \                |                 /
            Shared Memory Map, Signals, and Semaphores
                    \              |               /
                     v             v              v
               +----------------------------------------+
               |         /dev/shm/DungeonMem            |
               +----------------------------------------+

```

### 1. Game Engine & Supervisor (`game.c`)

Acts as the central control unit. It configures the virtual memory mapping table using raw file descriptors and invokes `ftruncate()` to expand memory to match the exact byte size of `struct Dungeon`. It executes consecutive process forks, performs `execv()` calls to swap child execution images with worker binaries, tracks live Process IDs (PIDs), and passes execution layout scopes to the core compiled driver library (`dungeon_X86_64.o`) by calling `RunDungeon(wizard_pid, rogue_pid, barbarian_pid)`.

### 2. The Barbarian Combat Component (`barbarian.c`)

Handles volatile integer evaluations. The process suspends its execution via the `pause()` system call to minimize CPU thread consumption. Upon intercepting an execution signal from the kernel, it immediately checks the shared memory space, reads the dynamically changing integer block `dungeon->enemy.health`, and assigns that value directly to `dungeon->barbarian.attack` to neutralize monster rooms before timeouts elapse.

### 3. The Wizard Cryptanalysis Component (`wizard.c`)

Manages string array cipher translation. When a magical barrier loads, the process parses an obfuscated char array (`dungeon->barrier.spell`). It implements a sliding-window Caesar Cipher routine that extracts the raw shift offset from index 0, routes non-alphabet punctuation and literal spaces safely through an unmodified bypass path, and handles boundary wrapping using modular arithmetic (`% 26`). The decrypted output string is committed to `dungeon->wizard.spell` via size-bounded memory copies to prevent buffer truncation warnings.

### 4. The Rogue Mechanism Component (`rogue.c`)

Executes high-frequency algorithmic searching. Facing mechanical traps, the Rogue process handles continuous binary search boundaries (`[0.0, 100.0]`) to hunt down random floating-point target pins. Every sub-tick loop computes a strict midpoint:


$$\text{mid} = \text{low} + \frac{\text{high} - \text{low}}{2}$$


It registers this guess directly into `dungeon->rogue.pick` and parses real-time character states written to `dungeon->trap.direction` by the engine:

* `'u'`: Target pin is higher $\rightarrow$ `low = mid`
* `'d'`: Target pin is lower $\rightarrow$ `high = mid`
* `'t'`: Sweet-spot convergence window met $\rightarrow$ loop exits successfully

---

## ⏱️ Execution & Runtime Timeline Breakdown

This section details how your custom code implementations interact seamlessly behind the scenes with the pre-compiled dungeon kernel engine to produce a perfect 360/360 sequence run.

### Phase 1: Initialization & Process Branching

```text
[Game Engine] Shared tables and locking pins established successfully.
[Wizard] Magical sensors calibrated.
[Barbarian] Standing by in the dungeon.
[Rogue] Hidden in shadows, awaiting signals.

```

* **Engine Bootstrapping:** `game.c` initializes first as the master orchestrator. It executes `shm_open()` to allocate a raw shared memory node under the string table label `"/DungeonMem"`. Using `ftruncate()`, it matches the memory geometry to the exact structural byte footprint of `struct Dungeon`, before layering a master pointer context using `mmap()`.
* **Lock Architecture:** `game.c` constructs the peripheral validation keys by invoking `sem_open()` twice. This sets up token checkpoints for `"/LeverOne"` and `"/LeverTwo"` with an initial counting baseline of `1`.
* **Process Branching:** The orchestrator invokes three back-to-back `fork()` calls:
* **Child 1** transforms into `wizard.c` through a clean `execv()` context replacement.
* **Child 2** replaces its memory layout with `barbarian.c`.
* **Child 3** replaces its memory layout with `rogue.c`.


* **Cross-Linking:** Every newly spawned child process mounts a local bridge to the identical segment using `shm_open()` and `mmap()`. Each configuration locks down low-level callback hooks through `sigaction()` configurations to sit suspended, parsing incoming kernel alerts (`DUNGEON_SIGNAL` / `SIGUSR1`).

### Phase 2: Handshaking with the Object Engine

```text
[Game Engine] Party assembled. Relinquishing core processing loop control to RunDungeon.
Verifying PID's... SUCCESS
All pid's valid. Attempting to open shared memory...: SUCCESS
Signal handling initialized in dungeon.

```

* **Control Transfer:** Once `game.c` verifies that its tracker registers are safely populated with fresh child variables, it drops out of its local loop and enters the compiled grading library block inside your professor's binary file: `RunDungeon(wizard_pid, rogue_pid, barbarian_pid)`.
* **Verification Loop:** The `RunDungeon()` runtime looks up the host `/proc` file subsystem to guarantee that all three child tracking PIDs are live, separate, and listening. It hooks a parallel evaluation pointer into `"/DungeonMem"` to activate synchronous state sharing.

### Phase 3: Dynamic Room Encounter Rounds (1 through 10)

#### Encounter A: The Monster Rooms

```text
This room has a monster in it!
[Barbarian] Battle signal received! Attacking enemy health point.
SUCCESS
The barbarian successfully incapacitated the monster!
Monster: 1012936258  | Barbarian: 1012936258

```

* **Trigger:** The core engine changes values inside `dungeon->enemy.health` and broadcasts an immediate alert via `kill(barbarian_pid, SIGUSR1)`.
* **Execution Response:** `barbarian.c` catches the signal and wakes from its low-power `pause()` state. Its custom signal interceptor scans the mapped shared segment layout, grabs the target data integer sitting in `health`, and matches it perfectly by copying it directly into `dungeon->barbarian.attack`.
* **Resolution:** The engine wakes up from an internal tracking delay, verifies mathematically that `attack == health`, prints a green `SUCCESS` marker to stdout, and logs your combat points.

#### Encounter B: The Caesar-Shift Barriers

```text
A barrier impedes your progress!
The barrier is blocked by an ancient incantation: LKmrfcp kyw G clrcp?
[Wizard] Magic barrier field detected. Deciphering glyphs...
[Wizard] Counter-spell focused: Mother may I enter?
SUCCESS
The wizard successfully brought down the magical barrier!

```

* **Trigger:** The engine updates the structural character matrix buffer `dungeon->barrier.spell` with an encoded cipher string. It formats the encryption rotation key explicitly into character position index `0` (e.g., `'L'`). It then invokes `kill(wizard_pid, SIGUSR1)`.
* **Execution Response:** `wizard.c` flags the signal and executes its corrected decryption algorithm loop. It extracts the structural modular key from index `0`, jumps directly into index `1` to isolate the payload, routes raw spacing spaces (`' '`) and punctuation cleanly around processing blocks, and passes the parsed translation back to `dungeon->wizard.spell` using bounded memory operations.
* **Resolution:** The core grading tracker runs a string equivalence check and validates a flawless bypass.

#### Encounter C: The Trap Interceptions (High-Frequency Micro-Loops)

```text
This room is guarded by a trap!
The rogue's pick is at position 0.000000 -> 52.000000, w
[Rogue] Trap mechanism encountered! Commencing binary lock-picking.
The rogue's pick is at position 50.000000 -> 52.000000, t
[Rogue] Lock cleared! Final position: 50.00

```

* **Trigger:** The engine registers a random float location pin target (e.g., `52.000000`) and dispatches `SIGUSR1` directly to the Rogue process.
* **Algorithmic Convergence:** This represents a high-speed iterative feedback engine. `rogue.c` sets up boundaries (`low = 0.0`, `high = 100.0`), calculates a center guess (`50.0`), and pushes it to `dungeon->rogue.pick` before calling `usleep()`.
* **Feedback Tracking:** The core engine reads the guess instantly. If the difference falls outside of `LOCK_THRESHOLD`, it posts direction indicators to `dungeon->trap.direction`:
* `'u'`: Guess too low $\rightarrow$ Rogue updates its scope: `low = mid`
* `'d'`: Guess too high $\rightarrow$ Rogue updates its scope: `high = mid`
* `'t'`: Target convergence zone resolved.


* As demonstrated in your micro-logs, the binary loop quickly cuts through variance scales (e.g., adjusting across steps from 50.0 to 25.0, down through 12.5, 6.25, and hitting 3.125) until the lock registers within safe tolerances.

### Phase 4: The Endgame Vault & Semaphores

```text
Behold, the door to the treasure has opened!
[Wizard] Semaphore signal caught! Securing Lever 2.
[Wizard] Lever 2 successfully held down.
[Barbarian] Semaphore signal caught! Securing Lever 1.
[Barbarian] Lever 1 successfully held down.
[Rogue] Party levers secured! Sweeping treasure chamber...

```

* **Phase Shift:** As soon as the final exploration phase wraps up, the engine issues a simultaneous broadcast of `SEMAPHORE_SIGNAL` (`SIGUSR2`) across all child processes.
* **Critical Lock Ingestion:**
* `barbarian.c` intercepts the signal and immediately halts on a blocking call to `sem_wait(leverOne)`.
* `wizard.c` intercepts the signal and halts on an identical blocking call to `sem_wait(leverTwo)`.
* These operations down-regulate the counting mechanisms from `1` to `0`, locking the physical vault entry lines in an open configuration.


* **Extraction:** `rogue.c` responds to `SIGUSR2`, systematically scanning `dungeon->treasure` byte-by-byte to collect the hidden characters (`s`, `w`, `i`, `m`) and writing them straight into `dungeon->spoils`.
* **Coordinated Drop:** The Barbarian and Wizard run background checks looking at `dungeon->spoils[3]`. As soon as the final index slot fills up with valid character values, they exit their observation loops, fire `sem_post()` to free up the system semaphores back to `1`, and terminate gracefully.

---

## 📂 Project Repository Structure

```text
├── .gitignore             # Excludes compiled target binaries, object files, and Windows OS metadata streams
├── Makefile               # Automated compilation script with decoupled building and linker-merging rules
├── README.md              # Project documentation, architectural breakdown, and operational blueprints
├── barbarian.c            # Source code for the Barbarian combat player process
├── dungeon_X86_64.o       # Core 64-bit pre-compiled grading driver binary engine provided by instructor
├── dungeon.o              # Renamed pre-compiled object target used during the linking stage
├── dungeon_info.h         # Layout definitions for the virtual structural memory blocks
├── dungeon_settings.h     # Hard limits (buffer constraints, threshold tolerances, and system signal IDs)
├── game.c                 # Primary process manager, virtual layout coordinator, and clean-up wrapper
├── mock_dungeon.c         # Local simulation harness used for testing decoupled structural routines
├── rogue.c                # Source code for the Rogue binary-searching trap mechanic process
└── wizard.c               # Source code for the Wizard cryptographic player process

```

---

## ⚙️ Core IPC Infrastructures

### 1. Virtual Shared Memory Allocation (`/dev/shm`)

The application coordinates global structures across separate physical memory maps by creating a virtual kernel file node under the system variable `dungeon_shm_name = "/DungeonMem"`.

* Shared layout registers are accessed concurrently by passing file descriptors from `shm_open(..., O_RDWR, 0666)` into the memory mapper:
```c
dungeon = (struct Dungeon*) mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

```


* Any structural change made by a player process inside the `mmap` perimeter instantly impacts the evaluation state of the central game matrix, bypassing standard file I/O latency.

### 2. Signal Routing Vector Map

Asynchronous communication relies on explicit POSIX user-defined signals routed via `sigaction()` configurations to maintain thread survival:

* **`DUNGEON_SIGNAL` (`SIGUSR1`):** Serves as an execution interrupt. When the core library loads a new trap, monster, or barrier room, it issues a `kill(target_pid, SIGUSR1)` broadcast to wake the dedicated child process.
* **`SEMAPHORE_SIGNAL` (`SIGUSR2`):** Signals the start of the final vault sequence. It shifts child processes out of their localized challenge configurations into a synchronized blocking loop.

### 3. POSIX Counting Semaphores

Endgame asset gathering requires strict thread synchronization to simulate cooperative leverage constraints. Access to the reward table is regulated via two system-named POSIX binary semaphores (`"/LeverOne"` and `"/LeverTwo"`):

* **The Claim Phase:** Upon intercepting `SIGUSR2`, the Barbarian down-regulates `"/LeverOne"` via `sem_wait()` while the Wizard down-regulates `"/LeverTwo"`. This pulls their values from `1` to `0`, signaling that the vault doors are physically pinned open.
* **The Extraction Window:** While the semaphores are securely downed, the Rogue sweeps the shared data space, copying sequential string keys from `dungeon->treasure` into `dungeon->spoils`.
* **The Release Phase:** The Barbarian and Wizard monitor `dungeon->spoils[3]`. As soon as the final index byte populates, they execute `sem_post()` to step out of the critical lock zones, ensuring the vault clear-out completes within the required timeout.

---

## 🚀 Building and Running

### System Dependencies

* **Processor:** x86_64 architecture (required by the pre-compiled ELF binary file `dungeon_X86_64.o`).
* **Toolchain:** GNU Compiler Collection (`gcc`), GNU Make tool suite (`make`).
* **Libraries:** Standard Real-Time extensions (`-lrt`) for shared memory maps and POSIX Multi-threading interfaces (`-pthread`) for semaphore threading checks.

### Operational Lifecycle Commands

### Step 1: Rename the Pre-compiled Object File

Before compiling, you need to copy or rename your `dungeon_X86_64.o` file into the target file name expected by your `Makefile` (`dungeon.o`). Run this in your terminal:

```bash
cp dungeon_X86_64.o dungeon.o

```

---

### Step 2: Clean Existing Build Artifacts

To prevent the linker or compiler from using stale configuration states or cached `.o` fragments from previous attempts, clear the workspace entirely:

```bash
make clean

```

---

### Step 3: Run the Clean Compilation Loop

Now, trigger your build automation script. Since your `Makefile` contains the `-Xlinker --allow-multiple-definition` flag, it will cleanly bind your custom character files directly to the newly linked `dungeon.o` engine without any duplicate global symbol conflicts:

```bash
make

```

---

### Step 4: Execute Your Program

Once compilation wraps up with no errors or warnings, you can start the parent orchestrator loop to test your implementation:

```bash
./game

```

---

### Step 5: Resource Cleaning & Kernel Scrubbing

The supervisor naturally handles cleanup when the simulation finishes. If manually terminating mid-run, issue a termination interrupt (`Ctrl+C`). The signal handlers intercept the interrupt and execute clean-up hooks using `shm_unlink()` and `sem_close()` to free all kernel resource tables.

```
***

```
