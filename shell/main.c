#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "executor.h"
#include "../commands/builtin.h"
#include "../diagnostics/diagnostic.h"

int main(void)
{
    char input[MAX_INPUT];

    OperationResult last_operation;

    memset(&last_operation, 0, sizeof(OperationResult));

    int should_exit = 0;

    printf("=================================\n");
    printf("        Welcome to LiteShell\n");
    printf("=================================\n");

    printf("Type 'help' for available commands.\n\n");

    while (!should_exit)
    {
        printf("LiteShell> ");
        fflush(stdout);

        /*
         * Read input
         */
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }

        /*
         * Remove newline
         */
        input[strcspn(input, "\n")] = '\0';

        /*
         * Ignore empty input
         */
        if (strlen(input) == 0)
        {
            continue;
        }

        /*
         * Handle LiteShell built-ins
         */
        int builtin_result =
            handle_builtin(
                input,
                &last_operation,
                &should_exit
            );

        if (builtin_result == 1)
        {
            continue;
        }

        /*
         * INSPECT command
         */
        if (builtin_result == 2)
        {
            DiagnosticResult diagnostic =
                diagnose_operation(&last_operation);

            print_diagnostic(&diagnostic);

            continue;
        }

        /*
         * Parse external command
         */
        Command command;

        if (parse_command(input, &command) == 0)
        {
            continue;
        }

        /*
         * Execute command
         */
        last_operation = execute_command(&command);

        /*
         * We deliberately do not print the
         * full diagnostic automatically yet.
         *
         * The user can request it with:
         *
         *     inspect
         */
        if (!last_operation.success)
        {
            printf(
                "LiteShell: operation failed. "
                "Use 'inspect' for details.\n"
            );
        }
    }

    printf("\nLeaving LiteShell.\n");

    return 0;
}
