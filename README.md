# IMCSH

## Introduction
IMCSH is a lightweight shell designed for Unix-based systems. It provides essential command-line functionality, including process management, input/output redirection, background execution, and user environment interactions.

This document serves as the official documentation, outlining the usage and implementation details of IMCSH.

## Installation and Compilation

### Prerequisites
IMCSH requires a Unix-like operating system (Linux, macOS), Windows is not and will not be supported.

### Compilation
C compiler (e.g., GCC, Clang) is needed for development.
IMCSH includes a `Makefile` for easy compilation. Run the following command to compile:
```sh
make
```

To clean up compiled files:
```sh
make clean
```

For immediate debugging:

``` sh
make debug
```
## Usage

### Running IMCSH
To start the shell, simply execute the imcsh bin, found in the build dir.

### Built-in Commands
IMCSH supports several built-in commands:
- `help`: Displays available commands.
- `globalusage`: Shows the version and authors.
- `exec <program> <args>`: Executes an external command.
- `jobs`: Lists background processes.
- `exit/quit`: Exits the shell, optionally terminating background processes.
- `cd <directory>`: Changes the current working directory.

### Input/Output Redirection
IMCSH supports redirection for output:
- `command > file`: Redirects standard output to `file` (overwrites if exists).
- `command >> file`: Appends standard output to `file`.

### Background Execution
Appending `&` to a command runs it in the background:
```sh
sleep 10 &
```
Use `jobs` to list background processes.

## Implementation Details
IMCSH is implemented in C, focusing on simplicity and modularity. The core components include:

### Process Management
- Uses `fork()` to create child processes.
- Uses `execvp()` to execute external commands.
- Manages background processes with `waitpid()`.

### Input Handling
- Tokenizes input using `strtok()`.
- Detects and handles special tokens such as `&`, `>`, and `>>`.

### Redirection Handling
- Uses `open()` with appropriate flags to handle file output.
- Uses `dup2()` to redirect output streams.

### Built-in Commands
- Implemented directly in the main shell loop.
- Custom handlers for `cd`, `exit`, and `jobs`.

## Future Enhancements
- Environment variable handling (e.g., `export`, `unset`, variable expansion).
- Additional redirection features (e.g., `2>` for stderr, `<` for input redirection).
- Command history and alias support.
- Arrow navigation through command history
- Ctrl+C, Ctrl+D, and other control combinations
- Improved job control (e.g., suspend/resume, foreground/background management).
- Tab completion and syntax highlighting.
- Ability to execute script files.
