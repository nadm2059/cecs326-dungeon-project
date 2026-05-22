
# Multi-Process Dungeon Crawler Simulator

A robust, concurrent terminal-based simulation application written in C that showcases advanced principles of UNIX Systems Programming and Inter-Process Communication (IPC). The system demonstrates high-performance parent-child orchestration, managing a dynamic party of completely autonomous worker processes (a Barbarian, a Wizard, and a Rogue). Synchronous operations are achieved via POSIX shared memory segments, real-time POSIX signal handling vectors, and counting semaphores to coordinate microsecond-accurate vault extraction phases.

---

## 🏗️ Architectural Blueprint & Process Roles

The application framework builds a distributed processing environment by allocating isolated kernel-level resource structures before executing concurrent child forks. [cite_start]Instead of threading within a single address space, true multi-processing via `fork()` ensures memory protection boundaries between game characters, which interface exclusively over explicit communication channels[cite: 3, 11, 12].

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

Acts as the central control unit. It configures the virtual memory mapping table using raw file descriptors and invokes `ftruncate()` to expand memory to match the exact byte size of `struct Dungeon`. It executes consecutive process forks, performs `execv()` calls to swap child execution images with worker binaries, tracks live Process IDs (PIDs) , and passes execution layout scopes to the core compiled driver library (`dungeon_X86_64.o`) by calling `RunDungeon(wizard_pid, rogue_pid, barbarian_pid)`.

### 2. The Barbarian Combat Component (`barbarian.c`)

Handles volatile integer evaluations. The process suspends its execution via the `pause()` system call to minimize CPU thread consumption. Upon intercepting an execution signal from the kernel, it immediately checks the shared memory space, reads the dynamically changing integer block `dungeon->enemy.health` , and assigns that value directly to `dungeon->barbarian.attack`  to neutralize monster rooms before timeouts elapse.

### 3. The Wizard Cryptanalysis Component (`wizard.c`)

Manages string array cipher translation. When a magical barrier loads, the process parses an obfuscated char array (`dungeon->barrier.spell`). It implements a sliding-window Caesar Cipher routine that extracts the raw shift offset from index 0 , routes non-alphabet punctuation and literal spaces safely through an unmodified bypass path, and handles boundary wrapping using modular arithmetic (`% 26`). The decrypted output string is committed to `dungeon->wizard.spell` via size-bounded memory copies to prevent buffer truncation warnings.

### 4. The Rogue Mechanism Component (`rogue.c`)

Executes high-frequency algorithmic searching. Facing mechanical traps, the Rogue process handles continuous binary search boundaries (`[0.0, 100.0]`) to hunt down random floating-point target pins. Every sub-tick loop computes a strict midpoint:


$$\text{mid} = \text{low} + \frac{\text{high} - \text{low}}{2}$$

It registers this guess directly into `dungeon->rogue.pick` and parses real-time character states written to `dungeon->trap.direction` by the engine:

* `'u'`: Target pin is higher $\rightarrow$ `low = mid`
* `'d'`: Target pin is lower $\rightarrow$ `high = mid`
* 
`'t'`: Sweet-spot convergence window met $\rightarrow$ loop exits successfully 



---

## 📂 Project Repository Structure

```text
├── .gitignore             # Excludes compiled target binaries, object files, and Windows OS metadata streams
├── Makefile               # Automated compilation script with decoupled building and linker-merging rules
├── README.md              # Project documentation, architectural breakdown, and operational blueprints
├── barbarian.c            # Source code for the Barbarian combat player process
[cite_start]├── dungeon_X86_64.o       # Core 64-bit pre-compiled grading driver binary engine [cite: 32]
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

* Shared layout registers are accessed concurrently by passing file descriptors from `shm_open(..., O_RDWR, 0666)`  into the memory mapper:


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

**The Claim Phase:** Upon intercepting `SIGUSR2` , the Barbarian down-regulates `"/LeverOne"` via `sem_wait()` while the Wizard down-regulates `"/LeverTwo"`. This pulls their values from `1` to `0`, signaling that the vault doors are physically pinned open.

**The Extraction Window:** While the semaphores are securely downed, the Rogue sweeps the shared data space, copying sequential string keys from `dungeon->treasure` into `dungeon->spoils`.

* **The Release Phase:** The Barbarian and Wizard monitor `dungeon->spoils[3]`. As soon as the final index byte populates, they execute `sem_post()` to step out of the critical lock zones, ensuring the vault clear-out completes within the required timeout.

---

## 🚀 Building and Running

### System Dependencies

**Processor:** x86_64 architecture (required by the pre-compiled ELF binary file `dungeon_X86_64.o`).
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


4. **Resource Cleaning & Kernel Scrubbing:**
The supervisor naturally handles cleanup when the simulation finishes. If manually terminating mid-run, issue a termination interrupt (`Ctrl+C`). The signal handlers intercept the interrupt and execute clean-up hooks using `shm_unlink()` and `sem_close()` to free all kernel resource tables.
---
