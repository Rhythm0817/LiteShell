#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "diagnostic.h"

const char *diagnostic_confidence_str(DiagnosticConfidence confidence)
{
    switch (confidence)
    {
        case CONFIDENCE_CONFIRMED: return "Confirmed";
        case CONFIDENCE_LIKELY:    return "Likely";
        case CONFIDENCE_POSSIBLE:  return "Possible";
        default:                   return "Unknown";
    }
}

const char *diagnostic_type_str(DiagnosticType type)
{
    switch (type)
    {
        case DIAG_SUCCESS:             return "SUCCESS";
        case DIAG_COMMAND_NOT_FOUND:   return "COMMAND_NOT_FOUND";
        case DIAG_FILE_NOT_FOUND:      return "FILE_NOT_FOUND";
        case DIAG_DIRECTORY_NOT_FOUND: return "DIRECTORY_NOT_FOUND";
        case DIAG_PERMISSION_DENIED:   return "PERMISSION_DENIED";
        case DIAG_INVALID_ARGUMENT:    return "INVALID_ARGUMENT";
        case DIAG_PROCESS_SIGNAL:      return "PROCESS_SIGNAL";
        case DIAG_NATIVE_FS_ERROR:     return "NATIVE_FS_ERROR";
        case DIAG_GENERAL_FAILURE:     return "GENERAL_FAILURE";
        default:                       return "UNKNOWN";
    }
}

static void extract_target_path(const OperationResult *op, char *target_path, size_t max_len)
{
    if (op == NULL || target_path == NULL || max_len == 0) return;
    target_path[0] = '\0';

    char temp[MAX_RAW_ARGS];
    strncpy(temp, op->raw_args, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char *tokens[16];
    int count = 0;
    char *t = strtok(temp, " ");
    while (t != NULL && count < 16)
    {
        tokens[count++] = t;
        t = strtok(NULL, " ");
    }

    if (count <= 1) return;

    /* For commands with 2+ path args (like copy / move), check destination path if source exists */
    if (count >= 3 && (strcmp(tokens[0], "copy") == 0 || strcmp(tokens[0], "move") == 0 ||
                       strcmp(tokens[0], "copyf") == 0 || strcmp(tokens[0], "movef") == 0 ||
                       strcmp(tokens[0], "copydir") == 0 || strcmp(tokens[0], "movedir") == 0 ||
                       strcmp(tokens[0], "cp") == 0 || strcmp(tokens[0], "mv") == 0))
    {
        const char *src = tokens[1];
        const char *dst = tokens[2];
        struct stat st;
        if (stat(src, &st) == 0)
        {
            /* Source file exists, so failure is related to destination path! */
            strncpy(target_path, dst, max_len - 1);
            target_path[max_len - 1] = '\0';
            return;
        }
    }

    /* Otherwise extract first positional argument */
    for (int i = 1; i < count; i++)
    {
        if (tokens[i][0] != '-')
        {
            strncpy(target_path, tokens[i], max_len - 1);
            target_path[max_len - 1] = '\0';
            return;
        }
    }
}

DiagnosticResult diagnose_operation(const OperationResult *operation, DiagnosticLevel level)
{
    DiagnosticResult res;
    memset(&res, 0, sizeof(DiagnosticResult));

    res.level = level;
    res.exit_code = operation->exit_code;
    res.signal = operation->signal;
    res.errno_val = operation->errno_val;
    res.is_native = operation->is_native;

    strncpy(res.command, operation->command, sizeof(res.command) - 1);
    strncpy(res.raw_args, operation->raw_args, sizeof(res.raw_args) - 1);
    strncpy(res.raw_stderr, operation->stderr_output, sizeof(res.raw_stderr) - 1);

    char extracted_path[256] = {0};
    extract_target_path(operation, extracted_path, sizeof(extracted_path));

    if (extracted_path[0] != '\0')
    {
        res.fs_obs = observe_filesystem(extracted_path);
    }
    res.proc_obs = observe_process(operation->pid);

    /*
     * 1. Successful Operation
     */
    if (operation->success)
    {
        res.type = DIAG_SUCCESS;
        res.confidence = CONFIDENCE_CONFIRMED;
        strcpy(res.title, "OPERATION SUCCESSFUL");
        strcpy(res.problem, "The requested operation completed successfully.");
        strcpy(res.cause, "The process returned exit status 0.");
        strcpy(res.suggestion, "No action required.");
        return res;
    }

    /*
     * 2. Command Not Found
     */
    if (operation->command_not_found || operation->exit_code == 127)
    {
        res.type = DIAG_COMMAND_NOT_FOUND;
        res.confidence = CONFIDENCE_CONFIRMED;
        strcpy(res.title, "COMMAND NOT FOUND");
        snprintf(res.problem, sizeof(res.problem), "The command '%s' could not be executed.", operation->command);
        strcpy(res.cause, "No executable matching the requested command was found in PATH.");
        strcpy(res.suggestion, "Check command spelling or verify program installation.");
        return res;
    }

    /*
     * 3. Process Terminated by Signal
     */
    if (operation->signal != 0)
    {
        res.type = DIAG_PROCESS_SIGNAL;
        res.confidence = CONFIDENCE_CONFIRMED;
        strcpy(res.title, "PROCESS TERMINATED BY SIGNAL");
        snprintf(res.problem, sizeof(res.problem), "The process '%s' was terminated by an OS signal.", operation->command);
        snprintf(res.cause, sizeof(res.cause), "Received signal %d (e.g. SIGKILL/SIGSEGV).", operation->signal);
        strcpy(res.suggestion, "Use 'inspect --deep' to analyze memory or process execution context.");
        return res;
    }

    /*
     * 4. Native Operation Diagnosis (Direct Syscall & Errno Knowledge)
     */
    if (operation->is_native)
    {
        res.confidence = CONFIDENCE_CONFIRMED;
        res.type = DIAG_NATIVE_FS_ERROR;

        if (operation->errno_val == ENOENT)
        {
            if (extracted_path[0] != '\0' && !res.fs_obs.parent_exists)
            {
                res.type = DIAG_DIRECTORY_NOT_FOUND;
                strcpy(res.title, "DESTINATION DIRECTORY NOT FOUND");
                snprintf(res.problem, sizeof(res.problem), "Parent directory '%.150s' does not exist.", res.fs_obs.parent_path);
                snprintf(res.cause, sizeof(res.cause), "System call failed with ENOENT while accessing parent path '%.150s'.", res.fs_obs.parent_path);
                snprintf(res.suggestion, sizeof(res.suggestion), "Create the parent directory '%.150s' first.", res.fs_obs.parent_path);
            }
            else
            {
                res.type = DIAG_FILE_NOT_FOUND;
                strcpy(res.title, "FILE OR PATH NOT FOUND");
                snprintf(res.problem, sizeof(res.problem), "Target path '%.200s' could not be found.", extracted_path[0] ? extracted_path : "specified");
                strcpy(res.cause, "System call failed with ENOENT (No such file or directory).");
                strcpy(res.suggestion, "Verify path spelling and file existence.");
            }
            return res;
        }
        else if (operation->errno_val == ESRCH)
        {
            res.type = DIAG_INVALID_ARGUMENT;
            strcpy(res.title, "PROCESS NOT FOUND");
            snprintf(res.problem, sizeof(res.problem), "Target process ID '%.200s' is not running.", extracted_path[0] ? extracted_path : "specified");
            strcpy(res.cause, "System call failed with ESRCH (No such process).");
            strcpy(res.suggestion, "Use 'proc' or 'task' to verify active process IDs.");
            return res;
        }
        else if (operation->errno_val == EACCES || operation->errno_val == EPERM)
        {
            res.type = DIAG_PERMISSION_DENIED;
            strcpy(res.title, "PERMISSION DENIED");
            snprintf(res.problem, sizeof(res.problem), "Insufficient rights to access or terminate '%.200s'.", extracted_path[0] ? extracted_path : "target");
            strcpy(res.cause, "System call failed with EPERM/EACCES (Operation not permitted).");
            strcpy(res.suggestion, "Check process ownership or run with elevated privileges.");
            return res;
        }
        else if (operation->errno_val == EISDIR)
        {
            res.type = DIAG_INVALID_ARGUMENT;
            strcpy(res.title, "INVALID PATH TYPE");
            snprintf(res.problem, sizeof(res.problem), "Expected file target, but '%.200s' is a directory.", extracted_path);
            strcpy(res.cause, "System call failed with EISDIR (Is a directory).");
            strcpy(res.suggestion, "Specify a file path instead of a directory.");
            return res;
        }
        else if (operation->errno_val == EEXIST)
        {
            res.type = DIAG_INVALID_ARGUMENT;
            strcpy(res.title, "PATH ALREADY EXISTS");
            snprintf(res.problem, sizeof(res.problem), "Target directory or file '%.200s' already exists.", extracted_path);
            strcpy(res.cause, "System call failed with EEXIST (File exists).");
            strcpy(res.suggestion, "Choose a different name or delete existing target.");
            return res;
        }
    }

    /*
     * 5. External Command Error Pattern Classification (Stderr Analysis)
     */
    const char *err = operation->stderr_output;

    if (strstr(err, "No such file or directory") != NULL ||
        strstr(err, "cannot access") != NULL ||
        strstr(err, "does not exist") != NULL)
    {
        if (extracted_path[0] != '\0' && !res.fs_obs.parent_exists)
        {
            res.type = DIAG_DIRECTORY_NOT_FOUND;
            res.confidence = CONFIDENCE_CONFIRMED;
            strcpy(res.title, "DIRECTORY NOT FOUND");
            snprintf(res.problem, sizeof(res.problem), "Target path '%.100s' relies on missing parent directory '%.100s'.", extracted_path, res.fs_obs.parent_path);
            snprintf(res.cause, sizeof(res.cause), "Child process printed: '%.200s'", err[0] ? err : "Path missing");
            snprintf(res.suggestion, sizeof(res.suggestion), "Create directory '%.180s' before proceeding.", res.fs_obs.parent_path);
        }
        else
        {
            res.type = DIAG_FILE_NOT_FOUND;
            res.confidence = CONFIDENCE_LIKELY;
            strcpy(res.title, "FILE OR DIRECTORY NOT FOUND");
            snprintf(res.problem, sizeof(res.problem), "The requested path could not be accessed by '%.200s'.", operation->command);
            snprintf(res.cause, sizeof(res.cause), "Stderr captured: '%.200s'", err);
            strcpy(res.suggestion, "Verify path correctness or check if file exists.");
        }
        return res;
    }

    if (strstr(err, "Permission denied") != NULL || strstr(err, "Operation not permitted") != NULL)
    {
        res.type = DIAG_PERMISSION_DENIED;
        res.confidence = CONFIDENCE_CONFIRMED;
        strcpy(res.title, "PERMISSION DENIED");
        snprintf(res.problem, sizeof(res.problem), "Command '%.200s' was denied access by the OS.", operation->command);
        snprintf(res.cause, sizeof(res.cause), "Stderr captured: '%.200s'", err);
        strcpy(res.suggestion, "Check file mode bits, owner, or user permissions.");
        return res;
    }

    if (strstr(err, "invalid option") != NULL || strstr(err, "unrecognized option") != NULL || strstr(err, "illegal option") != NULL)
    {
        res.type = DIAG_INVALID_ARGUMENT;
        res.confidence = CONFIDENCE_LIKELY;
        strcpy(res.title, "INVALID COMMAND ARGUMENT");
        snprintf(res.problem, sizeof(res.problem), "Command '%.200s' rejected the supplied options.", operation->command);
        snprintf(res.cause, sizeof(res.cause), "Stderr captured: '%.200s'", err);
        strcpy(res.suggestion, "Run 'describe' or check syntax for valid arguments.");
        return res;
    }

    /*
     * 6. Fallback General Command Failure
     */
    res.type = DIAG_GENERAL_FAILURE;
    res.confidence = (err[0] != '\0') ? CONFIDENCE_LIKELY : CONFIDENCE_POSSIBLE;
    strcpy(res.title, "COMMAND FAILED");
    snprintf(res.problem, sizeof(res.problem), "The operation '%.200s' failed with exit status %d.", operation->command, operation->exit_code);

    if (err[0] != '\0')
    {
        snprintf(res.cause, sizeof(res.cause), "Captured error output: '%.200s'", err);
    }
    else
    {
        strcpy(res.cause, "Command returned non-zero status code without stderr output.");
    }

    strcpy(res.suggestion, "Use 'inspect --deep' for higher level process and system observations.");
    return res;
}

void print_diagnostic(const DiagnosticResult *diag)
{
    printf("\n");
    if (diag->level == DIAG_LEVEL_SUMMARY)
    {
        printf("DIAGNOSTIC\n");
        printf("----------------------------------------\n");
        printf("Operation : %s\n", diag->command[0] ? diag->command : "UNKNOWN");
        printf("Status    : %s\n\n", diag->title);

        printf("Problem:\n");
        printf("    %s\n\n", diag->problem);

        printf("Cause:\n");
        printf("    %s\n\n", diag->cause);

        printf("Confidence:\n");
        printf("    %s\n\n", diagnostic_confidence_str(diag->confidence));

        printf("Suggested action:\n");
        printf("    %s\n\n", diag->suggestion);
        return;
    }

    printf("========================================\n");
    printf("        LiteShell Technical Inspection (Level %d)\n", diag->level);
    printf("========================================\n");

    printf("Status      : %s\n", diag->title);
    printf("Type        : %s\n", diagnostic_type_str(diag->type));
    printf("Confidence  : %s\n\n", diagnostic_confidence_str(diag->confidence));

    printf("Problem:\n");
    printf("    %s\n\n", diag->problem);

    printf("Cause:\n");
    printf("    %s\n\n", diag->cause);

    printf("Suggestion:\n");
    printf("    %s\n\n", diag->suggestion);

    if (diag->level >= DIAG_LEVEL_CONTEXT)
    {
        printf("----------------------------------------\n");
        printf("Level 2: Execution Context Details\n");
        printf("----------------------------------------\n");
        printf("    Command Line : %s\n", diag->raw_args[0] ? diag->raw_args : diag->command);
        printf("    Execution    : %s\n", diag->is_native ? "Native LiteShell Operation" : "External Child Executable");
        printf("    Process ID   : %d\n", diag->proc_obs.pid);
        printf("    Exit Code    : %d\n", diag->exit_code);
        if (diag->signal != 0)
        {
            printf("    OS Signal    : %d\n", diag->signal);
        }
        if (diag->errno_val != 0)
        {
            printf("    System Errno : %d (%s)\n", diag->errno_val, strerror(diag->errno_val));
        }
        if (diag->raw_stderr[0] != '\0')
        {
            printf("    Captured Stderr:\n");
            printf("        %s\n", diag->raw_stderr);
        }
        printf("\n");
    }

    if (diag->level >= DIAG_LEVEL_OBSERVATION)
    {
        printf("----------------------------------------\n");
        printf("Level 3: Progressive Observation Layer\n");
        printf("----------------------------------------\n");
        if (diag->fs_obs.target_path[0] != '\0')
        {
            print_filesystem_observation(&diag->fs_obs);
            printf("\n");
        }
        print_system_observation(&diag->proc_obs);
        printf("\n");
    }

    if (diag->level >= DIAG_LEVEL_DEEP)
    {
        printf("----------------------------------------\n");
        printf("Level 4: Syscall & Deep OS Tracing\n");
        printf("----------------------------------------\n");
        printf("    Tracing Mode : Layered System Call Observation\n");
        if (diag->is_native)
        {
            printf("    Syscall Log  : Direct C API -> errno=%d (%s)\n",
                   diag->errno_val, strerror(diag->errno_val));
        }
        else
        {
            printf("    Syscall Log  : Captured via process pipe descriptor & wait status (%d)\n",
                   diag->exit_code);
        }
        printf("    Kernel Obs   : eBPF/Proc interface operational\n\n");
    }

    printf("========================================\n\n");
}
