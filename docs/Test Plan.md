<h1 align="center" style="bold">Test Plan</h1>
One row per test. Each ticket's author adds rows as their tests land.

| Test name | What it's testing | Input | Expected output | Actual output |
|---|---|---|---|---|
| `test_route_get_negative` | `route_get()` with an id below the valid range | `ROUTE_UNKNOWN` | `NULL` | PASS |
| `test_route_get_valid` | `route_get()` with every valid id | Each id from `0` to `ROUTE_COUNT - 1` | A row whose path matches the id (for example `"/fast"` for `ROUTE_FAST`) | PASS |
| `test_route_get_too_large` | `route_get()` with an id above the valid range | `ROUTE_COUNT`, `ROUTE_COUNT + 5` | `NULL` | PASS |
| `test_route_from_path_exists` | `route_from_path()` with every real path | `"/fast"`, `"/slow"`, `"/bimodal"`, `"/api"`, `"/db"` | The matching route id | PASS |
| `test_route_from_path_missing` | `route_from_path()` with paths that are not routes | `"/nope"`, `""`, `"/fast/"`, `"/FAST"` | `ROUTE_UNKNOWN` | PASS |
| `test_get_service_ms_average` | Average service time for each real route | 10,000 samples per route, seed 42 | Mean within 20% of the expected mean (`mean_ms`, or 10.9ms for `/bimodal`) | PASS |
| `test_get_service_ms_range` | Jitter stays within its limit | Test route: 100ms, ±10% jitter; 10,000 samples | Every sample between 90ms and 110ms | PASS |
| `test_get_service_ms_same_seed` | The same seed gives the same sequence | Two seeds both set to 42; 100 samples each | Each pair of samples is equal | PASS |
| `test_get_service_ms_consecutive` | Each call moves the seed forward | Test route: 100ms, ±10% jitter; 100 samples | Not every sample equals the first | PASS |
| `test_get_service_ms_no_jitter` | No jitter is added when `jitter_pct` is 0 | Test route: 100ms, 0% jitter; 100 samples | Every sample is exactly 100ms | PASS |
| `test_get_service_ms_long_pct_10` | Long mode is picked at the `long_pct` rate | Test route: 100ms, long mode 1000ms at 10%, 0% jitter; 10,000 samples | 800 to 1,200 samples are 1000ms | PASS |
| `test_get_service_ms_long_pct_0` | Long mode is never picked at 0% | Test route: long mode at 0%; 10,000 samples | 0 samples are 1000ms | PASS |
| `test_get_service_ms_long_pct_100` | Long mode is always picked at 100% | Test route: long mode at 100%; 10,000 samples | All 10,000 samples are 1000ms | PASS |
| `test_service_route_invalid` | `service_route()` rejects ids that are not routes | `ROUTE_UNKNOWN`, `ROUTE_COUNT` | `ERROR` | PASS |
| `test_defaults_with_no_flags` | Default configs are set correctly | None - default values on no input | Each field in `config cfg` is set to expected value | PASS |
| `test_single_flag_leaves_other_fields_default` | Checking if values are properly overridden from default | `--threads 8` | Threads changes from 4 to 8, but no other fields are affected | PASS | 
| `test_<flagName>_flag` | Testing a change of every flag individually. So there are 12 individual tests. | All flags in `config` struct are targeted | Acceptance will be for each to pass | PASS | 
| `test_all_flags_together` | Testing every flag being changed during startup | All flags in `config` struct are targeted at once in one `argv[]` | Each field in config equal to `argv[]` value | PASS |
| `test_flag_order_does_not_matter` | Can input config flags in any order and the settings will be changed as expected | `quiet`, `threads`, `port` are targeted in 2 different orders | Both config parses result in the same values changed | PASS | 
| `test_<flagName>_boundaries_accepted` | Testing boundaries of each flag whether it be lower and upper int/double boundaries or min/max string length | Every flag is targeted | Boundary values are accepted | PASS |
| `test_<flagName>_rejected` | Testing multiple values that should be rejected and return a specific error | Every flag is targeted | Error returned (defined in `src/constants.h`) | PASS | 
| `test_unknown_flag_rejected` | Junk flags are rejected and result in errors returned | `--bogus`, `--bogus 1`, `--threads 4 --bogus` | `MALFORMED_ARGS` | PASS |
| `test_unknown_flag_rejected` | Missing values on valid flags are rejected as malformed | `--threads`, `--policy`, `--alpha`, `--logpath` | `MALFORMED_ARGS` | PASS | 
| `test_leftover_positional_args_rejected` | Leftover / extras on command line are rejected | `pool`, `--quiet extra`, `--threads 4 8`, `extra --quiet` | `MALFORMED_ARGS` | PASS | 
| `test_queue_pop_order` | Items pop in the order they were pushed | Capacity 3; push ids 0, 1, 2 | Pops return 0, 1, 2 | PASS |
| `test_queue_push_wraparound` | `head` and `tail` wrap past the end of the ring buffer | Capacity 3; 10 items, pushing one after each pop | Pops return 0 to 9 in order | PASS |
| `test_queue_push_blocks_when_full` | `queue_push()` blocks on a full queue until a slot frees | Capacity 2, full; a thread pushes 99, then main waits 100ms and pops once | Push has not returned after 100ms; then returns `QUEUE_OK`, and 99 pops last | PASS |
| `test_queue_pop_blocks_when_empty` | `queue_pop()` blocks on an empty queue until an item arrives | Capacity 2, empty; a thread pops, then main waits 100ms and pushes 42 | Pop has not returned after 100ms; then returns `QUEUE_OK` with 42 | PASS |
| `test_queue_spmc_exactly_once` | 1 producer and many consumers lose and duplicate nothing | Capacity 2; 1 producer pushes 10,000 ids; 2 × cores consumers (min 4) | Every id popped exactly once; each consumer's ids in increasing order | PASS |
| `test_shutdown_queue_wakes_poppers` | `shutdown_queue()` wakes every blocked popper | Empty queue; 2 × cores poppers blocked, then shutdown | Every pop returns `QUEUE_SHUTDOWN` | PASS |
| `test_queue_pop_drain` | Items left at shutdown are still popped | Capacity 3, full; shutdown; 4 pops | First 3 pops return 0, 1, 2; 4th returns `QUEUE_SHUTDOWN` | PASS |
| `test_shutdown_queue_wakes_pusher` | `shutdown_queue()` wakes a pusher blocked on a full queue | Capacity 2, full; a thread pushes 99, then shutdown | Push returns `QUEUE_SHUTDOWN` | PASS |
| `test_queue_pop_after_drained` | Pops after the queue is drained return at once | Empty queue; shutdown; 3 pops | Every pop returns `QUEUE_SHUTDOWN` without blocking | PASS |
| `test_shutdown_queue_rejected_pusher_keeps_items` | A pusher rejected at shutdown leaves queued items in place | Capacity 2, full; a thread pushes 99, then shutdown; 3 pops | Push returns `QUEUE_SHUTDOWN`; pops return 0, 1, then `QUEUE_SHUTDOWN` | PASS |
| `test_queue_try_push_full` | `queue_try_push()` rejects on a full queue without changing it | Capacity 2, full; try_push 99; shutdown; 3 pops | try_push returns `QUEUE_FULL`; pops return 0, 1, then `QUEUE_SHUTDOWN` | PASS |
| `test_queue_push_after_shutdown` | Pushes after shutdown are rejected | Capacity 3, empty; shutdown; push, try_push, pop | All three return `QUEUE_SHUTDOWN` | PASS |
| `test_queue_capacity_one` | The smallest ring buffer wraps on every operation | Capacity 1; 10 rounds of push, try_push, pop | Each push returns `QUEUE_OK`, each try_push returns `QUEUE_FULL`, each pop returns the pushed id | PASS |
| `test_queue_spmc_try_push` | Reject mode with 1 producer and many consumers | Capacity 2; 1 producer try_pushes 10,000 ids; 2 × cores consumers (min 4) | Every accepted id popped exactly once; no rejected id popped; each consumer's ids in increasing order | PASS |
| `test_initialize_queue_rejects_invalid` | `initialize_queue()` rejects bad arguments | Capacity 0; capacity `SEM_VALUE_MAX + 1`; policy `99` | `NULL` with `errno` set to `EINVAL` for each | PASS |
