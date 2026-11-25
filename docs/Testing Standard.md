<h1 align="center" style="bold">Testing Standard</h1>
This document will outline our projects' testing standard.\
**Contents:**
- [Testing Standard](#testing-standard)
  - [Where Tests "Live"](#where-tests-live)
  - [Test Naming Convention](#test-naming-convention)
  - [What Requires a Unit Test?](#what-requires-a-unit-test)
  - [Testing Multithreaded Code](#testing-multithreaded-code)
  - [Generating the Test Program](#generating-the-test-program)
  - [Expectations Before Merging PRs](#expectations-before-merging-prs)
  - [How Integration Testing Differs from Unit Testing](#how-integration-testing-differs-from-unit-testing)
## Where Tests "Live"
Tests live in the `tests/` directory of the executable they test (`src/[executable]/tests/`). Tests for shared code live in `src/common/tests/`.
This is where every test is contained whether it be a **unit test** or **integration** test file. The server's integration tests live in `src/server/tests/`.
## Test Naming Convention
For **modules**: `test_[module name].c`
> i.e. for `queue_fifo.c` the name would be `test_queue_fifo.c`

Within **module files** (the `test_[module name].c` files): `void test_[function name]_[type of test(optional)](void)`
> i.e. for testing a push on a full queue in the `queue_push()` function in `queue_fifo.c` the function name could be as follows below:
```C
/* test_queue_fifo.c */
void test_queue_push_full(void) {/* Code goes here */}
```

## What Requires a Unit Test?
Any modules (i.e. files) that have their own functionality. The list of modules that do not require testing is shorter than the list that do. For example, a `constants.h` file does not need to be tested as it does not have any functionality besides defining constants that are used throughout the program.

Any module should have **all** functions tested **unless** the function is a forwarding/wrapper function (i.e. only calls other functions). Then, it is acceptable to only test the functions that are called.

Code that only does socket I/O or thread setup (the listener, the worker pool) is tested by the integration tests instead of unit tests.

## Testing Multithreaded Code
Unity is **not thread-safe**. A failed assertion jumps back to the test runner with `longjmp`, so an assertion in a worker thread jumps into the main thread's stack and crashes or corrupts the test. In any test that starts threads:
- Worker threads only **record** what they did (e.g. into a per-thread array).
- The main thread **joins** every thread, then runs all assertions.

Concurrent tests should use more threads than cores and a small capacity (1 or 2) so the full and empty paths are actually hit.

A deadlocked test hangs instead of failing, so `make test` runs every test binary under `timeout`. A test that times out exits with status 124 and counts as a failure.

Tests for code shared between threads (the queues, the stats) must also pass under the TSan build (`make tsan`), since a race can pass a normal run and still be a bug.

## Generating the Test Program
Run `make test` from the project root to build and run every executable's tests. Running `make test` inside one `src/[executable]/` directory builds and runs only that executable's tests.
Continue running `make test` whenever changes are made to the test modules to see if your most recently written tests pass.

## Expectations Before Merging PRs
It is expected that **all** tests pass before merging the PR into `origin/main`. This is done to ensure we do not end up with errors and *known* bugs when attempting to compile a current, running, working version of the entire program.\
It is acceptable to make a PR and explicitly state the tests are not passing before creating a sub-branch from the test branch to make fixes to the module(s) that are not passing their tests.

It is also expected for all code to be up to the agreed upon code standard, which ideally is done when the PR is opened. If the code is updated in any way after tests had been created, it is necessary to re-run the tests to ensure they still pass. Static analysis is recommended to determine if behaviour may have changed even if tests are still passing.

## How Integration Testing Differs from Unit Testing
**Integration** testing refers to running a compiled version of the server and sending it real requests over sockets to ensure it is behaving in the expected way (i.e. serving routes, returning the right errors, handling more requests than workers, etc...). The integration test program starts the server itself, takes the server binary path as an argument so it can run against the normal and TSan builds, and stops the server when it finishes.\
**Unit** testing refers to testing individual modules and their functions. So rather than running the program, you create a test file (as outlined above) and test each function in the module using Unity in `/third_party/unity/`. Unity prints each test as it runs and whether it passed. If a test fails, the line it failed on will be printed along with the expected vs. actual values.
