# LiteShell OS - Build & Execution Guide

## Prerequisites

* **OS**: Linux or Windows Subsystem for Linux (WSL)
* **Compiler**: GCC (support for C99 / POSIX.1-2001 standard)
* **Build System**: GNU Make

---

## Build Instructions

### 1. Compiling LiteShell OS

To build the interactive `liteshell` binary and the evaluation test suite:

```bash
make clean && make
```

This compiles:
* `liteshell` - The primary CUI shell environment.
* `test_suite` - Automated empirical evaluation test runner.

---

## Running LiteShell OS

Launch the interactive shell:

```bash
./liteshell
```

### Sample Interactive Commands

```bash
# Display help and built-in options
LiteShell> help

# Execute normal Linux commands
LiteShell> pwd
LiteShell> ls -l

# Demonstrate native file operations with direct errno capture
LiteShell> copy nonexistent.txt /tmp/backup.txt
LiteShell: operation failed. Use 'inspect' for details.

# Level 1 Inspection (Human-readable summary)
LiteShell> inspect

# Level 2 Inspection (Execution context & stderr)
LiteShell> inspect --context

# Level 3 Inspection (Filesystem & /proc observation)
LiteShell> inspect --system

# Level 4 Inspection (Deep system call tracing)
LiteShell> inspect --deep

# Inspect operation history
LiteShell> history

# Self-describing command documentation
LiteShell> describe copy
```

---

## Running Automated Evaluation Test Suite

To run the full evaluation suite and display the diagnostic accuracy matrix:

```bash
make test
```

Or execute directly:

```bash
./test_suite
```
