# LiteShell OS - Architecture Specification

## 1. High-Level System Architecture

```text
                      LiteShell OS (v0.3)
                               │
             ┌─────────────────┼─────────────────┐
             │                 │                 │
        Core Commands        System         Diagnostics
             │             Monitor               │
       Filesystem          Overview         Failure Cause
       Operations          Memory           Context
       Directories         CPU / Load       Inspection
             │             Processes             │
             └─────────────────┼─────────────────┘
                               │
                        Operation Core
                               │
                        Operation Result
                               │
                       Observation Layer
                               │
               (Filesystem & /proc providers)
```

---

## 2. Component Subsystems

### 2.1 Parser (`shell/parser.h`, `shell/parser.c`)
Tokenizes user command strings into structured `Command` objects (`argc`, `args[]`).

### 2.2 Executor & Stderr Pipe Capture (`shell/executor.h`, `shell/executor.c`)
Manages execution of external Linux binaries (`fork()`, `execvp()`, `waitpid()`) using POSIX pipe descriptor capture (`pipe()`, `dup2()`).

### 2.3 Command Registry Manager (`core/command_registry.h`, `core/command_registry.c`)
Central registry manager storing command entries with names, aliases, descriptions, category (`CMD_CAT_FILESYSTEM`, `CMD_CAT_SYSTEM`, `CMD_CAT_DIAGNOSTICS`, `CMD_CAT_UTILITY`), syntax usage, and function handler pointers.

### 2.4 CUI System Monitor (`commands/monitor_ops.h`, `commands/monitor_ops.c`)
Integrated textual monitoring subsystem (`MONITOR` / `SYSMON`) providing:
* **System View**: Hostname, kernel release, architecture, uptime, system load averages (1m, 5m, 15m), active task count.
* **CPU View**: Processor model string, core count, load averages, user/system/idle ticks from `/proc/stat`.
* **Memory View**: Total, Used, Available, Free, Buffers/Cache, Swap Total/Free from `/proc/meminfo`.
* **Process Snapshot View**: Active PID, name, state, RSS memory usage.
* **Interactive Mode**: Textual menu mode (`MONITOR -i` / `MONITOR menu`).

### 2.5 Diagnostic Subsystem (`diagnostics/diagnostic.h`, `diagnostics/diagnostic.c`)
Cross-cutting LPDT diagnostic engine:
* `STATUS` - Operational status of latest or target operation.
* `DIAGNOSE` - Level 1/2 human-readable problem, cause, confidence (`Confirmed`, `Likely`, `Possible`), and suggested action.
* `INSPECT` - Level 2..4 progressive technical inspection.

### 2.6 Observation Providers (`observation/observation.h`, `observation/observation.c`)
* **Filesystem Provider**: POSIX `stat()` and `access()` checks inspecting target path existence, type, permissions, file size, owner UID/GID, and parent directory status.
* **System & Process Provider**: Reads `/proc/uptime`, `/proc/loadavg`, `/proc/meminfo`, `/proc/stat`, and `/proc/<pid>/` entries (`status`, `cmdline`, `cwd`).
