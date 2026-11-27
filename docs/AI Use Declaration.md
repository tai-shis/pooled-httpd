| Date | Tool | Used for | How verified | Errors found |
|---|---|---|---|---|
| November 24, 2026 | Claude | Test creation and finding/confirming edge cases for `config.c` | Reviewed code and verified that the tested inputs were following expected results (e.g. proper errors being thrown from `constants.h`) | No errors in tests, but did remove many redundant tests such as `print_config()` tests and those testing if the config retains memory (it never will since it can only be changed once upon startup) |
