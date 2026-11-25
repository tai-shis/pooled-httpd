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
