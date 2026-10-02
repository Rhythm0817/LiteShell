# LiteShell OS - Research Methodology (LPDT)

## 1. Research Motivation

Traditional Unix command-line shells expose low-level kernel and process error details directly to the user:
```
cp: cannot create regular file '/backup/test.txt': No such file or directory
```
Or exit with non-zero status codes without actionable guidance.

**LiteShell OS** introduces **Layered Progressive Diagnostic Transparency (LPDT)**, a methodology designed to:
1. Provide simple human-readable explanations by default.
2. Maintain progressive diagnostic depth accessible on demand (`inspect`, `inspect --deep`).
3. Classify error root causes using empirical evidence without claiming unverified intelligence.

---

## 2. The LPDT Diagnostic Pipeline

```
┌─────────────────────────────────────────────────────────┐
│                      1. Operation                       │
│           (User initiates command in LiteShell)         │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│                      2. Execution                       │
│     (Native system call execution or execvp child)     │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│                3. Event & Error Capture                 │
│    (Captured stderr pipe descriptor & system errno)     │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│              4. Failure Classification                  │
│       (Categorize: File, Dir, Perm, Cmd, Signal)        │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│                5. Context Collection                    │
│      (Observation Providers query stat and /proc)       │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│                 6. Cause & Confidence                   │
│     (Assess: Confirmed, Likely, Possible, Unknown)      │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│            7. Human-Readable Explanation                │
│       (Level 1: Problem, Cause, Suggestion, Conf)       │
└───────────────────────────┬─────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│              8. Progressive Inspection                  │
│   (Level 2: Context -> Level 3: Obs -> Level 4: Deep)   │
└─────────────────────────────────────────────────────────┘
```

---

## 3. Diagnostic Confidence Assessment

LiteShell distinguishes between different levels of empirical certainty:

| Confidence Level | Definition | Example Scenario |
| :--- | :--- | :--- |
| **Confirmed** | Empirical proof exists from direct system call `errno` or verified parent directory absence. | Native `copy` failed with `ENOENT` and `stat()` confirms parent directory does not exist. |
| **Likely** | Strong textual pattern match in captured `stderr` combined with partial filesystem observation. | External `ls` printed `"No such file or directory"` for a path missing on disk. |
| **Possible** | Inferred failure from process non-zero exit status without captured error output. | Process exited with status code `1` without writing to stderr. |
| **Unknown** | Uncategorized or unhandled execution anomaly. | Unexpected process status state. |

---

## 4. Native vs. External Operation Analysis

### External Commands (`execvp`)
* **Execution Model**: Process spawned in child process via `fork()` and `execvp()`.
* **Capture Mechanism**: Standard error (`stderr`) redirected via `pipe()` and `dup2()`.
* **Diagnostic Basis**: Stderr output pattern analysis + exit status + `stat()` / `access()` checks.

### Native Commands (`copy`, `move`, `remove`, `mkdir`, `cat`)
* **Execution Model**: Executed in-process directly using C POSIX system calls (`open`, `read`, `write`, `close`, `rename`, `unlink`, `mkdir`).
* **Capture Mechanism**: Immediate inspection of system `errno` variable.
* **Diagnostic Basis**: Exact system call error codes (`ENOENT`, `EACCES`, `EISDIR`, `EEXIST`, `ENOSPC`). Direct access to `errno` yields 100% `CONFIDENCE_CONFIRMED` diagnostics.

---

## 5. Evaluation Methodology

The diagnostic methodology is empirically validated using an automated benchmark test suite (`tests/test_suite.c`).

### Evaluation Metrics
1. **Diagnostic Accuracy**: Percentage of test cases correctly classified into target failure categories.
2. **Confidence Correctness**: Verification that confidence levels accurately reflect empirical evidence.
3. **Execution Overhead**: Millisecond execution latency added by pipe capture, stderr parsing, and observation queries.
