#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "../shell/executor.h"

typedef enum
{
    DIAG_SUCCESS,
    DIAG_COMMAND_NOT_FOUND,
    DIAG_PERMISSION_DENIED,
    DIAG_GENERAL_FAILURE,
    DIAG_PROCESS_SIGNAL
} DiagnosticType;

typedef enum
{
    CONFIDENCE_CONFIRMED,
    CONFIDENCE_LIKELY,
    CONFIDENCE_POSSIBLE,
    CONFIDENCE_UNKNOWN
} DiagnosticConfidence;

typedef struct
{
    DiagnosticType type;

    DiagnosticConfidence confidence;

    char title[128];

    char problem[256];

    char cause[256];

    char suggestion[256];

} DiagnosticResult;

DiagnosticResult diagnose_operation(OperationResult *operation);

void print_diagnostic(DiagnosticResult *diagnostic);

#endif
