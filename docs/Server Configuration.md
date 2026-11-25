# Server Configuration Flags

Usage:

```
./server [--flag value | --flag=value] ...
```

Every flag is optional. Anything you leave out keeps its default, and the full active config is printed to stderr at startup, so each run records what it tested.

## Flag reference

| Flag | Takes | Valid values | Default | Error if invalid |
|---|---|---|---|---|
| `--model` | string | `pool`, `tpr` | `pool` | `INVALID_MODEL_TYPE` (-2) |
| `--policy` | string | `fifo`, `priority` | `fifo` | `INVALID_QUEUE_TYPE` (-3) |
| `--threads` | integer | 1 to 256 | `4` | `THREAD_NUM_OOB` (-4) |
| `--capacity` | integer | 1 to 256 | `64` | `QUEUE_CAP_OOB` (-5) |
| `--port` | integer | 1024 to 65535 | `8080` | `PORT_RANGE_OOB` (-6) |
| `--alpha` | decimal | 0.0 to 1.0 | `0.5` | `ALPHA_VAL_OOB` (-7) |
| `--w` | decimal | 0.0 to 10.0 | `1.0` | `WEIGHT_VAL_OOB` (-8) |
| `--seed` | integer | 0 to 2147483647 (`INT_MAX`) | `1` | `SEED_VAL_OOB` (-9) |
| `--receive` | integer (ms) | 1 to 60000 | `5000` | `RCV_WAIT_OOB` (-10) |
| `--send` | integer (ms) | 1 to 60000 | `5000` | `SND_WAIT_OOB` (-11) |
| `--logpath` | string | any path up to 4095 characters | `server.log` | `LOGPATH_TOO_LONG` (-12) |
| `--quiet` | none | (a plain on/off switch) | off | n/a |

All bounds are inclusive: `--port 1024` and `--port 65535` are both accepted, and `--port 1023` and `--port 65536` are not.

## Examples

Defaults (FIFO, pool of 4 workers, queue of 64, port 8080):

```bash
./server
```

Priority queue with a larger weight on predicted length:

```bash
./server --policy priority --alpha 0.5 --w 1.5
```

FIFO control run for a priority experiment (w = 0 behaves as FIFO):

```bash
./server --policy priority --w 0
```

Thread-per-request baseline on its own port, with no stderr request lines:

```bash
./server --model tpr --port 9090 --quiet
```

Fully specified run for an experiment preset:

```bash
./server --model pool --policy priority --threads 8 --capacity 128 \
         --alpha 0.3 --w 1 --seed 42 --receive 2000 --send 2000 \
         --logpath e3_priority.log
```