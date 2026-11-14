<h1 align="center" style="bold">Programming Style Guide</h1>
This document will outline our projects' programming style.\
**Contents:**
- [Programming Style](#programming-style)
  - [Naming Convention](#naming-convention)
  - [Brace and Indentation Style](#brace-and-indentation-style)
  - [Comment Style](#comment-style)

## Programming Style
The programming style for this project is based on keeping the code readable, consistent, and easy to maintain. This is especially important because the project is split into separate executables under `/src/`, each made of modules with their public declarations in that executable's `include/`.

Each function should have one clear responsibility. Functions should return `SUCCESS` or `ERROR` where appropriate, and repeated functionality should be placed in a shared module instead of being copied between files. Code should also follow the conventions below so that files written by different contributors look like part of the same project.

## Naming Convention
Functions and variables use **snake_case**. Function names should describe the action being performed, while variable names should describe the value they store.
> i.e. `readline()`, `handle_error()`, `input_buffer`, and `bytes_read`

Constants and macros use **UPPER_SNAKE_CASE**. This includes project-wide values defined in `constants.h`.
> i.e. `MAX_BUFFER_SIZE`, `STDIN_FD`, `NULL_TERMINATOR`, and `EXIT_COMMAND`

Types and structs use **PascalCase** when a named type is introduced. The existing `struct Command` is an example of this convention.

Names should be descriptive and should avoid unnecessary abbreviations. A short name such as `i` is acceptable for a simple loop counter, but names such as `token_count` and `is_buffer_empty` should be used when a value has meaning outside a small loop.

New code should use `snake_case` consistently. For example, `is_buffer_full` should be preferred over `isBufferFull`.

## Brace and Indentation Style
This project uses **K&R brace style**. The opening brace is placed on the same line as the function, conditional, or loop declaration. The closing brace is aligned with the declaration that opened the block.
```C
int is_command_exit(struct Command command) {
    return (str_eq(command.argv[0], EXIT_COMMAND) == TRUE);
}
```

Code is indented using **four spaces** for each level. Tabs should not be used for indentation. Statements inside a function, conditional, or loop must be indented one level further than the surrounding block.
```C
while (s[len] != NULL_TERMINATOR) {
    len++;
}
```

There should be a space between a control statement and its opening parenthesis, such as `if (condition)` and `while (condition)`. Function calls do not use a space before their opening parenthesis, such as `print(message)`.

Keep related statements together and use blank lines to separate logical steps. Avoid unnecessary blank lines and avoid placing multiple statements on one line. Braces should be used for conditional and loop bodies, including bodies that currently contain only one statement.

## Comment Style
Comments should explain the purpose or reasoning behind code when that is not clear from the code itself. They should not restate a statement that is already self-explanatory.

Use `//` comments for short explanations inside implementation files. Use `/* ... */` when a comment needs to cover multiple lines.
```C
// Move the end pointer to the last character
while (*end != NULL_TERMINATOR) {
    end++;
}
```

Public functions declared in header files should use a Doxygen-style comment. These comments describe what the function does, its parameters, and its return value.
```C
/**
 * @brief runs the command specified by the input tokens
 * @param command the command to run
 *
 * @return 0 on success, or ERROR on failure.
 */
int run_command(struct Command *command);
```

Comments should be kept up to date when the code changes. Do not leave commented-out implementation in a committed module; remove it when it is no longer needed, since version control preserves previous versions of the code.
