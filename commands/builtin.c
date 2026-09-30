#include <stdio.h>
#include <string.h>

#include "builtin.h"

int handle_builtin(
    char *input,
    OperationResult *last_operation,
    int *should_exit
)
{
    /*
     * EXIT
     */
    if (strcmp(input, "exit") == 0)
    {
        *should_exit = 1;
        return 1;
    }

    /*
     * HELP
     */
    if (strcmp(input, "help") == 0)
    {
        printf("\n");
        printf("========================================\n");
        printf("             LiteShell Help\n");
        printf("========================================\n\n");

        printf("Built-in commands:\n\n");

        printf("  help\n");
        printf("      Display available LiteShell commands.\n\n");

        printf("  status\n");
        printf("      Display the result of the last operation.\n\n");

        printf("  inspect\n");
        printf("      Display diagnostic information about the last operation.\n\n");

        printf("  describe\n");
        printf("      Explain what a LiteShell command does.\n\n");

        printf("  exit\n");
        printf("      Exit LiteShell.\n\n");

        printf("External Linux commands can also be executed.\n\n");

        printf("Examples:\n");
        printf("  ls\n");
        printf("  pwd\n");
        printf("  whoami\n");
        printf("  echo hello\n");
        printf("  date\n\n");

        printf("========================================\n\n");

        return 1;
    }

    /*
     * DESCRIBE
     */
    if (strcmp(input, "describe") == 0)
    {
        printf("\n");
        printf("LiteShell Command Descriptions\n");
        printf("==============================\n\n");

        printf("inspect\n");
        printf("  Examines the result of the last operation\n");
        printf("  and explains the detected problem.\n\n");

        printf("status\n");
        printf("  Displays the basic execution result of\n");
        printf("  the last operation.\n\n");

        printf("describe\n");
        printf("  Displays information about LiteShell commands.\n\n");

        printf("help\n");
        printf("  Displays available commands.\n\n");

        printf("========================================\n\n");

        return 1;
    }

    /*
     * STATUS
     */
    if (strcmp(input, "status") == 0)
    {
        printf("\n");

        if (last_operation->command[0] == '\0')
        {
            printf("No operation has been executed yet.\n\n");
            return 1;
        }

        printf("Last Operation\n");
        printf("==============\n");

        printf("Command: ");
        printf("%s\n", last_operation->command);

        printf("Result: ");

        if (last_operation->success)
        {
            printf("SUCCESS\n");
        }
        else
        {
            printf("FAILED\n");
        }

        printf("Exit code: %d\n", last_operation->exit_code);

        if (last_operation->signal != 0)
        {
            printf("Signal: %d\n", last_operation->signal);
        }

        printf("\n");

        return 1;
    }

    /*
     * INSPECT
     */
    if (strcmp(input, "inspect") == 0)
    {
        if (last_operation->command[0] == '\0')
        {
            printf("\nNo operation is available for inspection.\n\n");
            return 1;
        }

        /*
         * The actual diagnostic engine will be called
         * from main.c.
         *
         * This command simply identifies itself as
         * a diagnostic request.
         */
        return 2;
    }

    return 0;
}
