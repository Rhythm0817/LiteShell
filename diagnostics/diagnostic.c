#include <stdio.h>
#include <string.h>

#include "diagnostic.h"

DiagnosticResult diagnose_exit_code(int exit_code)
{
    DiagnosticResult result;

    result.exit_code = exit_code;

    if (exit_code == 0)
    {
        result.type = DIAG_SUCCESS;

        strcpy(result.message, "Operation completed successfully.");
        strcpy(result.suggestion, "No action required.");
    }
    else if (exit_code == 127)
    {
        result.type = DIAG_COMMAND_NOT_FOUND;

        strcpy(result.message, "The requested command could not be found.");
        strcpy(result.suggestion,
               "Check the command name or PATH.");
    }
    else
    {
        result.type = DIAG_GENERAL_FAILURE;

        strcpy(result.message, "The command did not complete successfully.");
        strcpy(result.suggestion,
               "Further diagnostic analysis will be available in a future version.");
    }

    return result;
}

void print_diagnostic(DiagnosticResult result)
{
    printf("\n");
    printf("---- LiteShell Diagnostic ----\n");
    printf("Status: %s\n", result.message);
    printf("Suggestion: %s\n", result.suggestion);
    printf("Exit code: %d\n", result.exit_code);
    printf("------------------------------\n");
}
