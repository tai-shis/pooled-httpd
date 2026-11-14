<h1 align="center" style="bold">Programming Standard</h1>
This document will outline our projects' programming standard.\
**Contents:**
- [Programming Standard](#programming-standard)
  - [Thread Safety](#thread-safety)
  - [Concurrency](#concurrency)
  - [Error Handling](#error-handling)
  - [Named Constants over Magic Numbers](#named-constants-over-magic-numbers)
  - [Named Structures with Methods](#named-structures-with-methods)
  - [File Layout](#file-layout)

## Thread Safety
Every system call and library function used in the program must be checked for thread safety in its man page before use. The **ATTRIBUTES** section of the man page lists it as **MT-Safe** or **MT-Unsafe**.

An MT-Unsafe function may only be called when no other thread can call it at the same time, or must be replaced with its MT-Safe equivalent (usually the `_r` variant, e.g. `strerror_r()` over `strerror()`, `localtime_r()` over `localtime()`).

Every function used gets a row in the table below, added by whoever first uses it.

| Function | Attribute | Notes |
|---|---|---|
| | | |

Signal handlers may only call **async-signal-safe** functions (`man 7 signal-safety`). `printf()`, `malloc()`, and `pthread_mutex_lock()` are not async-signal-safe.

## Concurrency
**Every piece of shared state names its protection.** A struct shared between threads documents, next to its declaration, which lock protects which fields, or that a field is atomic. The critical sections table in the decision log lists the shared state and its protection; code must match it.

**Never block while holding a lock.** No socket I/O, `sleep`, or `printf` to a terminal while a mutex is held. The one exception is the bounded buffer semaphore pattern itself.

**Semaphore wait order:** always wait on the counting semaphore (`empty_slots` or `full_slots`) before `mutex`. Reversing the order deadlocks.

**Lock order:** code that holds more than one lock at a time must take them in the same order everywhere. Any such order is written down here.

**No shared RNG state.** Each thread has its own seeded RNG state.

## Error Handling
System calls signal failure by returning `ERROR` (`-1`) and setting `errno`. Two rules keep this consistent across the codebase:

**1. Check return values against the status constants**, not raw literals. `constants.h` defines `SUCCESS`, `ERROR`, `TRUE`, and `FALSE` for exactly this purpose, and every function that can fail should return one of them.
```C
if (listen(listen_fd, BACKLOG) == ERROR) {
    return handle_error("listen");
}
```

**2. Route every failure through a handler function** instead of printing inline at the call site. `handle_error(msg)` prints `msg` along with the reason from `errno`, and returns `ERROR`.

A caller should never need to know *how* a failure is reported, only that calling the handler is enough.

When a syscall fails, `errno` is the only source of *why* it failed, and the handler is the one place that should read it. `errno` is per-thread, so reading it in the handler is safe from any thread. The handler must turn `errno` into a message with `strerror_r()` or the `%m` format, never `strerror()`, which is MT-Unsafe.

## Named Constants over Magic Numbers
Any literal that is reused across files, or whose meaning is not obvious from context, must be a compile-time constant declared with `#define` in `constants.h`, named in `UPPER_SNAKE_CASE` per the [Programming Style Guide](Programming%20Style%20Guide.md#naming-convention).
```C
#define MAX_REQUEST_SIZE 4096
#define BACKLOG 128
#define DEFAULT_POOL_SIZE 8
```
Prefer `if (status == ERROR)` over `if (status == -1)`, and size buffers and arrays off a named constant rather than a bare number, so that changing a limit means changing one `#define` instead of hunting through every file that used the literal.

A value that is only ever used once, inside a single function, and whose meaning is already clear from the surrounding code (e.g. a loop step of `1`) does not need its own constant. Anything that appears in more than one file, sizes a buffer, or represents a fixed limit of the program belongs in `constants.h`.

## Named Structures with Methods
Data that is passed around the program as a unit (the work queue, the τ table, the stats, the config) is modeled as a named struct in its own header, declared in `PascalCase` per the style guide, with the functions that operate on it ("methods") declared alongside it in the same header. Code elsewhere should call those functions rather than reaching into the struct's fields directly.

When introducing a new structure that is reused across modules, follow the same shape: the struct lives in its own header, its methods are declared next to it, and it gets an `initialize_`/`free_` function if it owns any resources that need setup or teardown (memory, semaphores, mutexes, fds). If it is shared between threads, it also documents its protection per [Concurrency](#concurrency).

## File Layout
- **`src/[executable]/`** - one directory per executable (`server`, `loadgen`, `view`). Source files (`module_name.c`) sit directly in it, along with its Makefile. `main.c` stays limited to the program's entry point and orchestration; it delegates real logic to the other modules rather than implementing it inline.
- **`src/[executable]/include/`** - one header per module (`module_name.h`), containing that module's struct/type declarations and the prototypes for every function other modules are allowed to call.
- **`src/[executable]/tests/`** - unit test files, named `test_[module].c`. The server's integration tests also live here. See [Testing Standard.md](Testing%20Standard.md) for naming and coverage expectations.
- **`src/[executable]/bin/`** - build output (`.o` files and test binaries), one subdirectory per build variant (release, debug, TSan). It is not committed (see `.gitignore`).
- **`src/common/`** - code used by more than one executable (socket client, route names, snapshot format, time helpers), with the same internal layout. It is built as `libcommon.a` and linked into each executable.
- **`third_party/unity/`** - the vendored Unity test framework. It is kept separate from `src/` since it is not our code.
- **`docs/`** - project documents (this file, the style guide, the testing standard, the functionality document, the test plan).
- **`scripts/`** - `lab`, experiment presets, and the runner.
- **Project root** - the top-level `Makefile`, the shared `config.mk`, and the built `server`/`loadgen`/`view` binaries.

A new module always gets a matching pair: a header in `include/` and an implementation file with the same base name. A module with its own functionality also gets a `test_[module].c` in `tests/`, per the Testing Standard.
