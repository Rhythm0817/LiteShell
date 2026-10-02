#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "../shell/executor.h"
#include "../observation/observation.h"

typedef enum
{
    DIAG_SUCCESS,
    DIAG_COMMAND_NOT_FOUND,
    DIAG_FILE_NOT_FOUND,
    DIAG_DIRECTORY_NOT_FOUND,
    DIAG_PERMISSION_DENIED,
    DIAG_INVALID_ARGUMENT,
    DIAG_PROCESS_SIGNAL,
    DIAG_NATIVE_FS_ERROR,
    DIAG_GENERAL_FAILURE,
    DIAG_UNKNOWN
} DiagnosticType;

typedef enum
{
    CONFIDENCE_CONFIRMED,
    CONFIDENCE_LIKELY,
    CONFIDENCE_POSSIBLE,
    CONFIDENCE_UNKNOWN
} DiagnosticConfidence;

typedef enum
{
    DIAG_LEVEL_SUMMARY = 1,
    DIAG_LEVEL_CONTEXT = 2,
    DIAG_LEVEL_OBSERVATION = 3,
    DIAG_LEVEL_DEEP = 4
} DiagnosticLevel;

typedef struct
{
    DiagnosticType type;
    DiagnosticConfidence confidence;
    DiagnosticLevel level;

    char title[128];
    char problem[256];
    char cause[256];
    char suggestion[256];

    FilesystemObservation fs_obs;
    ProcessObservation proc_obs;

    char command[MAX_COMMAND_NAME];
    char raw_args[MAX_RAW_ARGS];
    char raw_stderr[MAX_STDERR_OUTPUT];
    int exit_code;
    int signal;
    int errno_val;
    int is_native;
} DiagnosticResult;

DiagnosticResult diagnose_operation(const OperationResult *operation, DiagnosticLevel level);
void print_diagnostic(const DiagnosticResult *diagnostic);
const char *diagnostic_confidence_str(DiagnosticConfidence confidence);
const char *diagnostic_type_str(DiagnosticType type);

#endif
