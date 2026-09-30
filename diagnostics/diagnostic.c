#include <stdio.h>
#include <string.h>

#include "diagnostic.h"

DiagnosticResult diagnose_operation(OperationResult *operation)
{
    DiagnosticResult result;

    memset(&result, 0, sizeof(DiagnosticResult));

    /*
     * Successful operation
     */
    if (operation->success)
    {
        result.type = DIAG_SUCCESS;
        result.confidence = CONFIDENCE_CONFIRMED;

        strcpy(
            result.title,
            "OPERATION SUCCESSFUL"
        );

        strcpy(
            result.problem,
            "The requested operation completed successfully."
        );

        strcpy(
            result.cause,
            "The command returned exit status 0."
        );

        strcpy(
            result.suggestion,
            "No action is required."
        );

        return result;
    }

    /*
     * Command not found
     */
    if (operation->command_not_found)
    {
        result.type = DIAG_COMMAND_NOT_FOUND;
        result.confidence = CONFIDENCE_CONFIRMED;

        strcpy(
            result.title,
            "COMMAND NOT FOUND"
        );

        snprintf(
            result.problem,
            sizeof(result.problem),
            "The command '%s' could not be executed.",
            operation->command
        );

        strcpy(
            result.cause,
            "No executable matching the requested command was found."
        );

        strcpy(
            result.suggestion,
            "Check the command name or verify that the program is in PATH."
        );

        return result;
    }

    /*
     * Process terminated by signal
     */
    if (operation->signal != 0)
    {
        result.type = DIAG_PROCESS_SIGNAL;
        result.confidence = CONFIDENCE_CONFIRMED;

        strcpy(
            result.title,
            "PROCESS TERMINATED"
        );

        strcpy(
            result.problem,
            "The process did not terminate normally."
        );

        snprintf(
            result.cause,
            sizeof(result.cause),
            "The process was terminated by signal %d.",
            operation->signal
        );

        strcpy(
            result.suggestion,
            "Use deeper inspection to determine why the process was terminated."
        );

        return result;
    }

    /*
     * General command failure
     */
    result.type = DIAG_GENERAL_FAILURE;
    result.confidence = CONFIDENCE_LIKELY;

    strcpy(
        result.title,
        "COMMAND FAILED"
    );

    snprintf(
        result.problem,
        sizeof(result.problem),
        "The command '%s' did not complete successfully.",
        operation->command
    );

    if (operation->error_message[0] != '\0')
    {
        strncpy(
            result.cause,
            operation->error_message,
            sizeof(result.cause) - 1
        );
    }
    else
    {
        strcpy(
            result.cause,
            "The command returned a non-zero exit status."
        );
    }

    strcpy(
        result.suggestion,
        "Use 'inspect' for additional diagnostic information."
    );

    return result;
}

void print_diagnostic(DiagnosticResult *diagnostic)
{
    printf("\n");
    printf("========================================\n");
    printf("        LiteShell Diagnostic\n");
    printf("========================================\n");

    printf("Status:\n");
    printf("    %s\n\n", diagnostic->title);

    printf("Problem:\n");
    printf("    %s\n\n", diagnostic->problem);

    printf("Cause:\n");
    printf("    %s\n\n", diagnostic->cause);

    printf("Suggestion:\n");
    printf("    %s\n\n", diagnostic->suggestion);

    printf("Confidence:\n");

    switch (diagnostic->confidence)
    {
        case CONFIDENCE_CONFIRMED:
            printf("    Confirmed\n");
            break;

        case CONFIDENCE_LIKELY:
            printf("    Likely\n");
            break;

        case CONFIDENCE_POSSIBLE:
            printf("    Possible\n");
            break;

        default:
            printf("    Unknown\n");
            break;
    }

    printf("========================================\n\n");
}
