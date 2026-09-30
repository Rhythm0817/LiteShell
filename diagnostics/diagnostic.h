#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

typedef enum
{
    DIAG_SUCCESS,
    DIAG_COMMAND_NOT_FOUND,
    DIAG_FILE_NOT_FOUND,
    DIAG_PERMISSION_DENIED,
    DIAG_GENERAL_FAILURE
} DiagnosticType;

typedef struct
{
    DiagnosticType type;
    int exit_code;
    char message[256];
    char suggestion[256];
} DiagnosticResult;

DiagnosticResult diagnose_exit_code(int exit_code);

void print_diagnostic(DiagnosticResult result);

#endif
