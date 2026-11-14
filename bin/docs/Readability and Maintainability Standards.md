<h1 align="center" style="bold">Readability and Maintainability Standards</h1>
This document will outline our projects' readability and maintainability standards.\
**Contents:**
- [Readability and Maintainability Standards](#readability-and-maintainability-standards)
  - [Guard Clauses over Nested Conditionals](#guard-clauses-over-nested-conditionals)
  - [Small, Named Helper Functions](#small-named-helper-functions)
  - [Simple Loops over Clever Ones](#simple-loops-over-clever-ones)
  - [A Function Should Read as What It Does](#a-function-should-read-as-what-it-does)
  - [Clarity over Tricks](#clarity-over-tricks)
  - [No Dead or Silently-Swallowed Code](#no-dead-or-silently-swallowed-code)

## Guard Clauses over Nested Conditionals
Handle the exceptional or invalid case first, return immediately, and let the rest of the function run unindented. This keeps the "main path" of a function at a single level of nesting instead of buried inside an `if`.
```C
void handle_child_process(struct Command *command) {
    if (command->argv[0] == NULL) {
        return;
    }

    char absolute_path[256];
    ...
}
```
`is_command_exit()` and `get_absolute_path()` follow the same shape: each independent failure or edge case gets its own early `return`, in sequence, rather than being nested inside an `else`.
```C
int get_absolute_path(const char *input_path, char *path) {
    if (file_exists(input_path) == TRUE) {
        str_cpy(path, input_path);
        return 0;
    }

    if (is_relative_path(input_path) == TRUE) {
        relative_path_to_absolute(input_path, path);
        return file_exists(path) ? 0 : -1;
    }

    return check_env_paths(input_path, path);
}
```
`if`/`else if` chains are reserved for genuinely mutually-exclusive alternatives (e.g. matching one of several environment variable names in `initialize_env()`), not used as a substitute for guard clauses. As a rule of thumb, a function body should not need more than two levels of indentation; if it does, look for a condition that can be turned into an early return instead.

## Small, Named Helper Functions
When a loop body or a function accumulates a piece of logic that is only one or two lines but has a distinct purpose, pull it out into its own named function. The caller then reads as a sequence of intentions instead of a block of raw statements.
```C
void insert_argument(char *argv[], const char *token, int arg_count) {
    argv[arg_count] = alloc(MAX_TOKEN_SIZE);
    str_cpy(argv[arg_count], token);
}

void reset_arg_buffer(char *arg_buffer, int *arg_index) {
    arg_buffer[0] = NULL_TERMINATOR;
    *arg_index = 0;
}
```
These two, plus `add_char_to_buffer()`, are each only a couple of lines, but pulling them out of `tokenize_input()` means its loop body reads as `add_char_to_buffer(...)`, `insert_argument(...)`, `reset_arg_buffer(...)` - the steps of the algorithm - rather than raw array indexing. `digit_to_char()` in `lib.c` is the same idea: a single-line expression given a name so the three `itoa_*` variants that use it don't repeat `'0' + digit` inline.

A helper is worth extracting when it has a name that is clearer than the code it replaces, or when the same one or two lines would otherwise be duplicated across functions. A single arithmetic expression used once, with an already-obvious meaning, does not need its own function.

## Simple Loops over Clever Ones
Loops walk an index or pointer toward an obvious sentinel (`NULL_TERMINATOR`, `LF`, `:`) using `while`, since most of what this shell parses is not naturally bounded ahead of time.
```C
unsigned int str_len(const char *s) {
    unsigned int len = 0;

    while (s[len] != NULL_TERMINATOR) {
        len++;
    }

    return len;
}
```
Use a `for` loop instead when a bound already exists before the loop starts, such as iterating `environ` in `initialize_env()`. Use `do`/`while` when the loop body must run at least once before the exit condition can be checked, such as extracting the first digit in `itoa_int()` before checking whether any digits remain.

Prefer a single exit condition in the loop header over `break`s scattered through the body. `readline()`'s single `break`, taken right after setting `is_buffer_empty`, is the acceptable case: the loop cannot express "stop, but remember why" purely through its condition. A loop with more than one `break`, or a `break` alongside a `continue`, is a sign the loop is doing more than one job and should be split into a helper or restructured around its exit condition.

## A Function Should Read as What It Does
A reader should be able to tell what a function does from its name and its first few lines, without stepping through every branch. This falls out of two things this project already asks of every function: one clear responsibility, and a name that is a verb phrase describing that responsibility (`flush_buffer`, `relative_path_to_absolute`, `handle_error_with_memory_cleanup`).
```C
int run_command(struct Command *command) {
    pid_t pid;

    pid = fork();

    switch (pid) {
        case ERROR:
            return handle_error_with_memory_cleanup("Error occurred while forking process");
        case 0:
            handle_child_process(command);
            _exit(0);
        default:
            int status;
            waitpid(pid, &status, 0);
    }

    return 0;
}
```
`run_command()` forks, waits, and returns - the actual work of the child process is delegated to `handle_child_process()` rather than inlined here. A function that forks *and* execs *and* handles three different error cases in one body is harder to summarize in one sentence than one that delegates each concern; if you can't describe what a function does in a single sentence, it is probably doing more than one thing and should be split.

Keep functions short enough to read in one screenful. Most functions in this codebase are under ~15 lines; a function noticeably longer than that is worth checking for a piece that can be extracted, per [Small, Named Helper Functions](#small-named-helper-functions).

## Clarity over Tricks
Prefer the obvious way of expressing a condition or value over a compact one that requires the reader to decode it. Compare against named status constants (`ERROR`, `TRUE`, `FALSE`), per the [Programming Standard](Programming%20Standard.md#named-constants-over-magic-numbers), rather than raw literals - `if (status == ERROR)` over `if (status == -1)`.

Bit-twiddling, nested ternaries, and pointer arithmetic beyond what a loop already requires are avoided. The one ternary in the codebase, in `get_absolute_path()`, is a plain condition mapping to two named outcomes:
```C
return file_exists(path) ? 0 : -1;
```
not a nested or compound one. The `print()` macro's use of `_Generic` in `output.h` is the deliberate exception: it exists to give callers one ergonomic overload (`print(x)` dispatching by the type of `x`) instead of forcing them to remember `print_int()` vs. `print_str()` vs. `print_size_t()` at every call site, and it is documented with a Doxygen comment right where it's declared. A trick is acceptable when it removes a decision from every caller and is clearly explained; it is not acceptable when it only saves the author a few keystrokes at the cost of the reader's time.

If a loop or block cannot be made to read cleanly, don't leave a comment apologizing for it - restructure it, or pull the awkward part into a named helper per [Small, Named Helper Functions](#small-named-helper-functions), so the code and its explanation are the same thing.

## No Dead or Silently-Swallowed Code
Do not leave commented-out code paths in a committed module - this is already stated in the [Programming Style Guide](Programming%20Style%20Guide.md#comment-style), and applies just as much to a block of speculative logic as to a single line:
```C
// if (WIFEXITED(status)) {
//     println("Child process exited normally");
// } else if (WIFSIGNALED(status)) {
//     println("Child process terminated by signal");
// }
```
Version control already preserves this history; delete it instead of commenting it out. If the logic is genuinely needed later, write it and test it, rather than leaving a guess in place.

Similarly, don't discard a failure status by reassigning a `status`/`result` variable multiple times without checking it, since only the last assignment's outcome is ever visible to the caller:
```C
int status = SUCCESS;

status = print(GREEN_COLOR);
status = print(get_user());
status = print(PURPLE_COLOR);
...
return status;
```
Each call here can independently fail, but only the last one's result survives. Follow the [Error Handling](Programming%20Standard.md#error-handling) standard instead: check (or route through a handler) at each call that can fail, rather than overwriting a status variable and hoping the last write was the one that mattered.
