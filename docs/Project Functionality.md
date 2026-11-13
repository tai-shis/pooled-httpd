# Overview

The core idea is to build a multi-threaded HTTP server in C with a fixed thread pool to act as the "workers". These threads handle requests based on a bounded work queue.\
We will compare the pooled thread approach to a request-by-thread approach, where each incoming TCP connection request spawns a new thread. This benchmark is important because of the idea that high thread-count multi-threaded servers using a thread-by-request approach tend to perform poorly in practice due to the overhead created by context switching. Lock contention will also be benchmarked alongside the threads to compare how each approach affects wait time for shared resources.
## Deliverable
- A server that accepts connections, queues them, and runs routes on the threads in two models (thread-per-request vs. thread pool), alongside two queuing policies (FIFO, predicted-service-time priority queue)
- Instrumentation compiled into the server that records queue wait, service time, CPU time, lock wait time, and context switch overhead
- Measurement of client-side latency using a seeded load-generator
- Six experiments to link results to course concepts such as:
  - threads
  - CPU scheduling and scheduling algorithms
  - critical sections
  - semaphores
  - deadlock
  - producer-consumer
  - pipes
  - sockets
  - signals

## Memo Feedback
| Feedback Point | Response |
|----------------|----------|
| **Project builds on course VM** | We will handle full project and module-specific builds through a `makefile` script. Each member of the group does use some kind of Linux distribution, so we do not anticipate any issues with compilation on the VM just as in Project #1. However, VM compilation will be periodically, likely once per week to be absolutely certain we are on track.|
| **Route behaviour** | We choose to implement both CPU-bound loop and sleep behaviour for routing. Given your suggestion, we want to compare the results of both of these behaviours. *More information outlined below*.  |
| **Processes and IPC - Where fork, exec, pipe, dup2, waitpid are used** | Only in the C version of `./lab`. Core of the project uses the shell. *More information outlined below*.|
| **Visualization**| C for `./view` and `./lab`. Python with `matplotlib` for the data plots|
| **Core and optional stretch**| Core and optional lists are deliberately kept apart and are outlined in the **Scope** section.|
| **Test plan**| Unit and integration test using `Unity` testing framework. Information outlined in the **Quality Assurance and Testing** section.|
| **Division of work**| Just as in Project #1, we are using tickets to divide labour. Each group member communicates and assigns tickets to themselves based on their programming ability and/or desire to work on a given project function.<br>Obviously, not all tickets will have equal amounts of work, so we are not explicitly measuring "lines of code" or "number of tickets assigned" in any way.|

## Route Behaviour (CPU-bound & Sleeping Routes)
Both route types have been chosen because we can run experiements to compare the two and show our results.\
- **CPU-bound routes** spin in a loop. Service time depends on how much demand for CPU time there is vs. how much is actually available (CPU contention). This is a measurable outcome and can be compared between service time and CPU time
- **Sleeping routes** use `nanosleep` (such as waiting on a disk or database to return a request). A worker that is "sleeping" uses no CPU time, so other threads can take it's place, using CPU time it cannot make use of.
- **Expected effect(s):** With CPU-bound routes, more threads cannot increase throughput, so the pool and thread-per-request are expected to perform similarly when load is **low**. This is because demand is not outpacing available resources. When load grows, thread-per-request is expected to suffer due to overhead of thread creation and context switching. With sleeping routes, threads overlap the waiting and should raise throughput until resource limits are reached.

### Single-core VM Consequences
Due to the single-core limitation, running tasks in parallel is not an option. Threads will create an obvious overhead due to context switches, but no parallel speedup will be observed. The only gain we can make is through sleeping routes as the sleeping route is not allowed to take up CPU time if there is another thread waiting for CPU time. 

## Processes and IPC
The core of the project will uses signals, pipes, sockets, but `fork()` and `execve()` are left to the shell.
- Pipe: shell connects `./server` with `./view` -> `./server | ./view`
- Sockets: load generator talks to the server over HTTP
- Signals: SIGINT gracefully shuts the server down and SIGPIPE (invalid write to pipe when read end is closed) is ignored so we can handle it properly without shutting down the program instantly.
- Shell: launches processes, so `fork()`, `execve()`, `pipe()`, `dup2()`, and `waitpid()` are not called by our code

Since `./lab` is an optional addition, the calls to things such as `pipe()` and `dup2()` will only appear if it is implemented. 

## Visualization
We wil start with the `./view` and `./lab` module running the visualization in C. `ncurses` is a potential upgrade of the visualization (time permitting). Graphs are made from the CSV output in Python and **matplotlib**. We will confirm it is on the VM, and **gnuplot** is the backup if we cannot use it.  

## Division of Work
Ticketing system on **Linear** is used just as in Project #1. The ticket author writes that ticket's tests and runs `make test` to ensure tests are passing before merging any code. Tickets follow a phase schedule.
- Server and queue first
- thread-per-request
- instrumentation
- load generator
- experiments
- live view
- report
- slides 

# Core System
There will be three executables:
1. server
2. load generator
3. live view

The server and load generator run as separate processes.

## Architecture
Load generator sends HTTP requests to the server. There is an `acceptor` (producer) which sends this request to the queue. The connection waits in the queue until a worker can receive it. A stats thread prints snapshots that our `./view` can read from a pipe.

The repository layout is as follows:
- One directory per executable under `src/` (`server`, `loadgen`, `view`), each with its own Makefile, `include/`, `tests/`, and `bin/`.
- `src/common/` holds code used by more than one executable (socket client, route names, snapshot format, time helpers), built as `libcommon.a`.
- Top-level Makefile builds common first, then each executable; shared flags live in config.mk.
- Outputs and extras: built binaries land in the repo root; Unity is vendored in `third_party/unity/`; docs are in `docs/`; scripts (lab, presets, runner) are in `scripts/`.

## Server Functionality
The server (`./server`) accepts one connection at a time, queues it, and lets a worker run the matching route; each connection carries exactly one request (`Connection: close`).

- **Thread Pool (core):** fixed number of worker threads take connections from the bounded queue
- **Thread-per-Request (baseline):** one new thread per connection, uncapped (to show when it fails). Server will return "503 Service Unavailable" error upon failing.

### Queue policies
- FIFO: a ring buffer.
- Priority: a min-heap on priority = enqueue_time + w * tau[route], computed once at enqueue; the lowest value is served first. It approximates non-preemptive shortest-job-first on predicted per-route service times, so we call it "priority" rather than SJF.
- tau: a per-route exponential average of wall-clock service time, tau_next = alpha * t + (1 - alpha) * tau, with tau0 = 0 (the first sample sets tau = t). Defaults: alpha = 0.5, w = 1. The memo suggested that w near 1.5 may work better, so the w sweep in E3 includes it.
- Aging: the enqueue_time term gives aging, so there is no separate starvation check. w = 0 behaves as FIFO; a larger w is more SJF-like with weaker aging.
- No preemption: a job runs to completion once dequeued.
- Reading the request: in priority mode the acceptor reads the request line itself (bounded by the read timeout) to learn the route. Known limitation: a slow client delays other accepts; our load generator does not do this.
- Queue item: one struct for both policies (fd, route, enqueue_time, priority); FIFO ignores priority.

### Queue synchronization
The queue is the textbook bounded buffer with three semaphores: mutex (1), empty_slots (capacity), and full_slots (0), identical for FIFO and priority. Producers and consumers always wait on empty_slots or full_slots before wait(mutex); reversing that order deadlocks, and we use it as our deadlock example. For shutdown we set the flag, then post full_slots once per worker; workers check the flag after waking and before dequeuing.

| Route | Type | Service Time | Used In |
| ----- | ---- | ------------ | ------- |
| `/fast` | CPU | ~1ms, +/- 10% | Experiment #2 and #3 |
| `/slow` | CPU | ~50ms, +/- 10%|Experiment #2 and #3 |
| `/bimodal` | CPU | 90% ~1ms, 10% ~100ms| Experiment #3 (predictor failure) |
| `/drift` | CPU | mean shifts mid-run | Experiment #3 (alpha sweep) |
| `/api` | Sleep | ~20ms | Experiment #2 (sleep-heavy) |
| `/db` | Sleep | ~10ms, +/- 50%  | none yet  |
### Routes (synthetic)
CPU routes spin through a calibrated amount of work, so CPU contention shows up as latency. Sleep routes use nanosleep (blocked, no CPU) to simulate network calls. Variance comes from a seeded per-thread RNG.

### Robustness
• HTTP parsing: request line only; read until \r\n\r\n is complete, handling split reads. Respond 400 for malformed requests, 404 for unknown routes, 405 for the wrong method.
• Socket timeouts: SO_RCVTIMEO and SO_SNDTIMEO, so a silent client cannot hold a worker forever.
• Syscall errors: retry on EINTR; loop on short reads and writes; back off on EMFILE and ENFILE; close on EPIPE (SIGPIPE is ignored).

## Instrumentation, Load Generator, Live View
Instrumentation is compield into the server and times every request with CLOCK_MONOTONIC, reporting overall averages as well as per route averages.

| Interval | From->To | What it shows |
| -------- | -------- | ------------- |
| Queue wait | enqueue -> dequeue | FIFO vs. priorirty, convoy effect, starvation |
| Service time | dequeue -> done | Feeds tau; shows CPU contention
| Server residence | accept -> done | Total time the server holds a request |
| Lock wait | before lock -> acquired | Contention |

We also record CPU time per request (`CLOCK_THREAD_CPUTIME_ID`), so service time minus CPU time is the time a worker was off the CPU. Voluntary and involuntary context switches come from `getrusage`, which is read *once* at *shutdown*. Client-side latency is measured by the load generator.
- **Status:** one shraed struct under a mutex, used by both models. Each thread updates it once per request
- **Worker states:** each worker writes its entry, no lock. The stats thread is responsible for reading them
- **Stats thread:** every ~100ms it locks the status struct to copy it. Prints after copy is complete
- **Overhead check:** `-DSTATS=-` compile-time switch used to measure the instrumentation overhead.
- **Output streams:** `stdout` carries line-buffered snapshots (t=12.3 rps=840 q=17/64 lockwait=3.1%...). Request lines, errors, and startup config go to `stderr`. Every request is logged to a file. CSV summaries are computed from request logs.
- **Headless operation:** server never draws UI itself, so benchmarks run the same binary with no reader. If the reader stops reading the pipe, only the stats thread blocks, not the workers.

## Load generator (Part 3)
`./loadgen` is a separate, seeded, closed-loop client: C concurrent connections, each sending its next request when the previous one completes, so load is set by C rather than an arrival rate. It records send-to-response latency per request in its own request log and discards the first N seconds of each run as warm-up. Its knobs are C, duration, and seed; route mix weights are fixed in code. To test a full queue, C must exceed pool size plus queue capacity, because a closed loop never puts more than C requests in the server. On a multi-core machine, client and server are pinned to separate cores with `taskset`.

## Live View and Lab Script
`./server [flags] | ./view` connects the two through a shell pipe. The view parses snapshot lines and redraws a text dashboard:
- **Worker states:** one cell per worker (Running, Idle, Blocked, Lock) in pool mode; the live thread count in thread-per-request mode.
- **Queue:** occupancy against capacity, and the active policy.
- **Per-route table:** current tau, average service time, average queue wait, and request count.
- **Totals:** requests per second, lock wait %, contended vs. uncontended acquisitions, and the 503 count.
`./lab` is a shell script for the core system: it takes a preset and runs `./server | ./view` alongside `./loadgen`. When the load generator finishes, the script sends **SIGINT** to the server; the server drains and exits, and the view exits on pipe EOF.

## Thread Safety and Critical Sections
We identified eight sections of shared state. There is a protection for each; the work queue is the only one that uses the semaphore buffer.

| State # | Shared State | Writers / Readers|
| ------- | ------------ | ---------------- |
| 1 | Work queue (ring buffer or heap) | producer enequeues, workers dequeue |
| 2 | Per-route tau | Workers write, producer read at enqueue |
| 3 | Shutdown flag | Signal path writes, workers read |
| 4 | Stats| All threads write after each request. Stats thread reads and `view` can display it|
| 5 | RNG State| Every worker|
| 6 | Logging| All threads|
| 7 | Live thead count (thread-per-request) | Thread create and exit |
| 8 | Worker States | Each worker writes its own entry; stats thread reads the entry which can then be used by `view` |

### Course Requirements we are Following
- **Thread-safe library usage:** before using any system call or library function, we check its man page ATTRIBUTES section for MT-Safe or MT-Unsafe and record each verdict in our [Programming Standards](./Programming%20Standard.md) document 
- **Signal handlers:** they call only async-signal-safe functions (`man 7 signal safety` for information)
- **Deadlock avoidance:** the queue always waits on `empty_slots` or `full_slots` before `mutex`, and ThreadSanitizer and Helgrind check ordering of the locks.

## Experiments
We have six experiments to produce evidence for the report and presentation. Every run uses a fixed seed, excludes any warm-up window, 3-5 repeats, if the machine has more than one core, pins client and server to separate cores. Again, on the VM this is not possible, so it will not happen and context-switching will create additional overhead as outlined before.
| Experiment # (E) | Experiment | Variable(s) | What we are measuring | Expected Result | 
| ------------ | ---------- | ----------- | --------------------- | --------------- |
| 1 | Thread-per-request vs. pool | C (concurrent clients) | throughput, avg. latency, thread count, memory, failures  | Similar when C is low, thead-per-request degrades as C grows and then fails when C is large |
| 2 | Pool size vs. cores | Pool size; CPU-heavy vs. sleep-heavy | Throughput, service minus CPU time gap, involuntary context switches | On a single-core, CPU-heavy throughput peaks at a small pool and proceeds to fall as extra threads add context switch overhead. Sleep-heavy benefits from more threads |
| 3 | Queue policy | FIFO vs. Priority: sweep the weight and alpha  | Average wait per route | priority lowers `/fast` wait and raises `/slow`. Smaller 'w' caps `/slow` wait time. `/bimodal` shows prediction failure. `/drift` shows how fast tau adapts per alpha |
| 4 |Queue structure | Ring buffer (FIFO) vs. heap (priority) | Queue lock wait, contended &%, throughput | The heap lengthens the critical section and raises the lock wait | 
| 5 | Full-queue behaviour | Queue capacity. Block vs. reject | Average latency, Rate of 503 errors returned | Larger queue means fewer errors but higher latency. Reject policy will put an upper-bound on the latency
| 6 | Observer effect | Live view attached vs. headless. `stderr` vs `--quiet` flag | Throughput, latency | Almost no  difference  

E1 runs with ulimit -n raised so threads run out before file descriptors do. The E2 routes are `/fast`, `/slow`, and `/api`; E3 uses `/fast`, `/slow`, `/bimodal`, and `/drift`.
Course concepts the experiments demonstrate:
- Threads and resource limits: E1 shows creation cost, thread explosion, and EAGAIN.
- CPU scheduling: E2 shows runnable threads against cores and preemption of CPU routes.
- Scheduling algorithms: E3 shows FCFS vs. SJF-style prediction, the convoy effect under FIFO, burst prediction by exponential averaging, and aging.
- Critical sections and semaphores: E4 compares lock wait for the ring buffer and the heap.
- Producer-consumer: E5 shows queue occupancy and block vs. reject on a full queue.
- IPC (pipes, sockets, signals): the live view uses a pipe, the load generator talks to the server over sockets, and SIGINT drives graceful shutdown.

# Quality Assurance and Testing
Three tools are checking concurrency. Each run becomes a test plan entry.\
**Example:**\
"TSan, 64 Concurrent clients, 0 reports"

| Tool | Catches | Notes |
| ---- | ------- | ----- |
| ThreadSanitizer (`-fsanitize=thread`) | Data races with line of the source | Separate build (`make tsan`) |
| Helgrind (Valgrind) | Races, lock-order problems (catching potential deadlocks), `pthread` misuse | Runs the normal binary |
| Valgrind memcheck with `--track-fds=yes` | Leaks, use-after-free, uninitialized reads, fd leaks | One run covers memory and fds |

- **Unit tests:** Unity, vendored once in `third_party/unity/`, with one test program per module.
- ****Integration tests:** also Unity; a C test program starts `./server`, connects over real sockets, and checks the responses. They live in `src/server/tests/`.
- **Concurrency checks:** unit and integration tests also run under TSan, Helgrind, and memcheck.
- **Ownership and timing:** the author of a ticket writes that ticket's tests, and we run make test by hand before merging.
- **Debuggability:** threads are named with `pthread_setname_np` (`acceptor`, `worker-03`, `stats`, `signal`).
- **Make targets:** `make`, `make clean`, `make debug`, `make tsan`, `make test`. Release, debug, and TSan builds use separate output directories, so TSan also covers the shared `common` library

# Scope: Optional Features, Dropped Items, Out of Scope

**Optional features (only if time permitting)**:
- ./lab rewritten as a C program (using fork, execv, waitpid, pipe, and dup2), then a full ncurses lab
- Live controls, such as changing the queue policy mid-run
- A --mix flag for the load generator
- Capped thread-per-request
- Per-worker queues
- An epoll-based model
- Histograms and percentiles

**Dropped from the original idea:** `/heavy`, `/chain`, a `/db` connection pool (and its experiment E7), the cond synchronization variant, config-file parsing, per-worker stats slots, a separate CSV writer, /`cpu?n=`, `/flaky`, open-loop load, replay mode, a `/stats` route, a monitor thread inside the server, and shared-memory export.

**Out of scope:** HTTPS, POST bodies, chunked encoding, keep-alive, full HTTP/1.1 compliance, and changing settings mid-run (unless live controls are completed).

# Open decisions, schedule, and change control
**Five design decisions remain open and will be settled in this document as we make them**
- [ ] Signal handling: a dedicated `sigwait` thread or a flag handler
- [ ] How the acceptor is woken at shutdown: close or `shutdown()` the listen fd, or a self-pipe
- [ ] Default full-queue policy, block or reject (both are built and toggled by a flag for E5)
- [ ] Alpha and w sweep values (week 3)
- [ ] Warm-up window length for benchmarks

**Already decided:** priority mode reads the request line in the acceptor; one queue item struct serves both policies; tau has its own mutex; defaults are alpha = 0.5 and w = 1; and work is split into tickets that anyone on the team can take.

## Schedule
1.  Nov 9 to 15: server (acceptor, parsing, routes, pool), FIFO and priority queue on semaphores, minimal request logging.
2.  Nov 16 to 22: thread-per-request, shutdown, snapshots and stats, load generator.
3.  Nov 23 to 29: experiments and live view; choose the presentation slot (Nov 23 to 27).
4.  Nov 30 to Dec 4: report and slides; deliverables due Fri Dec 4, 11:59 p.m.; presentations Dec 7 to 11.