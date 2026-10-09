# LiteShell OS

> **A Minimalist Linux-Based Command-Oriented Operating Environment with Integrated Layered Diagnostics & Responsive CUI**

LiteShell OS is an undergraduate CSE research prototype exploring the design of a disciplined, command-oriented operating environment inspired by DOS-era CLI simplicity, lightweight server administration tools, responsive terminal interfaces, and modern progressive observability concepts.

---

## 🌟 Core Philosophy

```
  SIMPLE BY DEFAULT
  RELIABLE DURING NORMAL OPERATION
  OBSERVABLE WHEN NEEDED
  DETAILED ON DEMAND
```

Diagnostics in LiteShell OS is a **core system capability**, but not its sole purpose. The system functions as a lightweight, clean command-oriented environment while providing cross-cutting diagnostic transparency whenever operations fail.

---

## 🏗️ Architecture Overview

```
                      LiteShell OS
                           │
             ┌─────────────┼─────────────┐
             │             │             │
       Native Commands   System     Diagnostics
             │         Management        │
       Filesystem      Processes      Failure Cause
       Operations      Memory         Context
       Directories     Resources      Inspection
             │             │             │
             └─────────────┼─────────────┘
                           │
                    Operation Core
                           │
                    Operation Result
                           │
                   Observation Layer
                           │
               Responsive Terminal CUI
              (ioctl, SIGWINCH, Screen Engine)
```

---

## 💻 Restrained & Responsive Terminal CUI

LiteShell OS features a responsive terminal interface (`ui/terminal.h/.c`, `ui/screen.h/.c`) that queries terminal dimensions dynamically using `ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)` and handles terminal window resize events via a thread-safe `SIGWINCH` signal handler flag.

* **Expanded Mode (Wide Terminal, >= 80 columns)**: Renders complete system header (host, uptime, memory, task count, date/time, CWD) and detailed multi-column tables (`NAME`, `TYPE`, `SIZE`, `MODIFIED` for files; `PID`, `NAME`, `STATE`, `UID`, `THREADS` for processes).
* **Compact Mode (Small Terminal, < 80 columns)**: Gracefully collapses headers and tables without line wrapping or corrupted layout borders.

Startup interface example:

```text
+--------------------------------------------------------------------------------+
| LiteShell OS v0.4                             Date: 02-10-2026  Time: 17:30:15 |
| Host: workstation   Uptime: 00:14:05  Mem: 475/3877 MB  Tasks: 33              |
| CWD: /home/user/projects                                                       |
+--------------------------------------------------------------------------------+

System ready. Type HELP for a list of native commands.

----------------------------------------------------------------------------------
 HELP: Command reference   MONITOR: System status   DIAGNOSE: Explain failure   EXIT: Quit
----------------------------------------------------------------------------------

LITESHELL>
```

---

## 🚀 Native Command Vocabulary & Subsystems

LiteShell uses a clean, native command naming convention that distinguishes file and directory operations explicitly.

### 1. File Operations

* `makef <file>` - Create a file using native POSIX `open()` (`O_WRONLY|O_CREAT|O_EXCL`).
* `delf <file>` - Delete a file using native POSIX `unlink()`.
* `copyf <src> <dst>` - Copy a file using `open()`, `read()`, `write()`, `close()`.
* `movef <src> <dst>` - Move or rename a file using `rename()`.
* `typef <file>` - Display file contents to standard output.

### 2. Directory Operations

* `dir [path]` - Responsive directory contents listing (wide vs. compact column layout).
* `makedir <dir>` - Create directory using native POSIX `mkdir()`.
* `deldir <dir>` - Delete empty directory using native POSIX `rmdir()`.
* `copydir <src> <dst>` - Copy directory and files.
* `movedir <src> <dst>` - Move or rename directory using native POSIX `rename()`.

### 3. Navigation Operations

* `cdir <path>` - Change working directory using native `chdir()`.
* `curdir` - Display current directory path using native `getcwd()`.

### 4. System & Monitoring

* `sysinfo` - System information (kernel release, hostname, architecture, CPU model, uptime).
* `mem` - Memory information (total, used, free, available, buffers/cache, swap).
* `proc [--all]` - Process information snapshot (responsive column layout).
* `monitor [mem|cpu|sys|proc|menu]` - Interactive/overview CUI system monitor.

### 5. Diagnostics

* `status [id]` - Display result of most recent operation.
* `diagnose [id]` - Explain failure cause in human-readable terms.
* `inspect [id] [--deep|--system|--context]` - Multi-level technical inspection.

### 6. Control & Utilities

* `help` - Display full LiteShell native command reference & alias list.
* `history` - Command execution history ring buffer.
* `clear` - Clear terminal screen.
* `date` - Display current system date.
* `time` - Display current system time.
* `about` - Display LiteShell information & architecture.
* `exit` - Exit LiteShell.

### 7. Advanced Parsing, Redirection & Signals

* **Quotation Handling**: Full support for single (`'...'`) and double (`"..."`) quotes allowing file paths with whitespace (e.g., `makef "/path/with space.txt"`).
* **Standard I/O Redirection**:
  * `> <file>` - Redirect stdout to file (create/truncate).
  * `>> <file>` - Redirect stdout to file in append mode.
  * `< <file>` - Redirect stdin from file.
  * Compatible across both in-process native commands (`dir > list.txt`, `proc >> log.txt`) and external child executables.
* **Variable & Exit Code Expansion**: Supports `$VAR`, `${VAR}`, `$?` (previous command exit code), and `$$` (shell PID).
* **Signal Resilience**: Interactive `SIGINT` (Ctrl+C) handling prevents terminal aborts, preserving prompt state.

### 8. Backward Compatibility Aliases

Standard Unix/DOS aliases are supported transparently in the command registry:
`cd` -> `cdir`, `pwd` -> `curdir`, `ls` -> `dir`, `cat` -> `typef`, `cp` -> `copyf`, `mv` -> `movef`, `rm` -> `delf`, `mkdir` -> `makedir`, `rmdir` -> `deldir`.

---

## 🛠️ Build & Execution

```bash
# Build LiteShell OS and test suite
make clean && make

# Launch LiteShell OS
./liteshell

# Run automated evaluation test suite
make test
```

---

## 📊 Evaluation Test Suite Results

```text
========================================================================================
         LiteShell OS - Native Command & Responsive CUI Evaluation Suite
========================================================================================

Test Case                      | Expected Diagnosis   | Actual Diagnosis     | Confidence | Time(ms) | Result 
----------------------------------------------------------------------------------------
1. Native Curdir (pwd)         | SUCCESS              | SUCCESS              | Confirmed  |     0.01 | PASS
2. Native System Info          | SUCCESS              | SUCCESS              | Confirmed  |     0.12 | PASS
3. Native Memory Stats         | SUCCESS              | SUCCESS              | Confirmed  |     0.04 | PASS
4. Native Directory List       | SUCCESS              | SUCCESS              | Confirmed  |    34.60 | PASS
5. Native Task Process List    | SUCCESS              | SUCCESS              | Confirmed  |     1.34 | PASS
6. Native Task Info            | SUCCESS              | SUCCESS              | Confirmed  |     0.21 | PASS
7. Native Make File            | SUCCESS              | SUCCESS              | Confirmed  |     0.04 | PASS
8. Native Type File            | SUCCESS              | SUCCESS              | Confirmed  |     0.02 | PASS
9. Native Copy File            | SUCCESS              | SUCCESS              | Confirmed  |     0.05 | PASS
10. Native Move File           | SUCCESS              | SUCCESS              | Confirmed  |     0.03 | PASS
11. Native Delete File         | SUCCESS              | SUCCESS              | Confirmed  |     0.03 | PASS
12. Native Make Directory      | SUCCESS              | SUCCESS              | Confirmed  |     0.02 | PASS
13. Native Change Directory    | SUCCESS              | SUCCESS              | Confirmed  |     0.01 | PASS
14. Native Delete Directory    | SUCCESS              | SUCCESS              | Confirmed  |     0.01 | PASS
15. System Monitor Overview    | SUCCESS              | SUCCESS              | Confirmed  |     0.73 | PASS
16. System Monitor Process View| SUCCESS              | SUCCESS              | Confirmed  |     0.75 | PASS
17. Command Not Found          | COMMAND_NOT_FOUND    | COMMAND_NOT_FOUND    | Confirmed  |   134.58 | PASS
18. Native File Not Found      | FILE_NOT_FOUND       | FILE_NOT_FOUND       | Confirmed  |     0.06 | PASS
19. Native Dir Not Found       | DIRECTORY_NOT_FOUND  | DIRECTORY_NOT_FOUND  | Confirmed  |     0.02 | PASS
20. Native Copy Missing Src    | FILE_NOT_FOUND       | FILE_NOT_FOUND       | Confirmed  |     0.01 | PASS
21. Native Copy Missing Dir    | DIRECTORY_NOT_FOUND  | DIRECTORY_NOT_FOUND  | Confirmed  |     0.03 | PASS
22. Native Mkdir Already Exists| INVALID_ARGUMENT     | INVALID_ARGUMENT     | Confirmed  |     0.01 | PASS
23. Permission Failure         | PERMISSION_DENIED    | PERMISSION_DENIED    | Confirmed  |     0.01 | PASS
24. Quoted Path Makef          | SUCCESS              | SUCCESS              | Confirmed  |     0.04 | PASS
25. Quoted Path Typef          | SUCCESS              | SUCCESS              | Confirmed  |     0.01 | PASS
26. Redirection Out (>)        | SUCCESS              | SUCCESS              | Confirmed  |     0.20 | PASS
27. Redirection Append (>>)    | SUCCESS              | SUCCESS              | Confirmed  |     0.07 | PASS
28. Quoted Path Delf           | SUCCESS              | SUCCESS              | Confirmed  |     0.02 | PASS
----------------------------------------------------------------------------------------
Evaluation Summary: 28 / 28 Tests Passed (100.0% Accuracy)
========================================================================================
```
