#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "executor.h"
#include "../commands/builtin.h"

int main(void)
{
    char input[MAX_INPUT];

    printf("=================================\n");
    printf("        Welcome to LiteShell\n");
    printf("=================================\n");
    printf("Type 'help' for available commands.\n\n");

    while (1)
    {
        printf("LiteShell> ");
        fflush(stdout);

        /*
         * Read user input
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
         * Built-in command: exit
         */
        if (strcmp(input, "exit") == 0)
        {
            break;
        }

        /*
         * Built-in commands
         */
        if (handle_builtin(input))
        {
            continue;
        }

        /*
         * Parse command
         */
        Command command;

        if (parse_command(input, &command) == 0)
        {
            continue;
        }

        /*
         * Execute command
         */
        OperationResult result = execute_command(&command);

        /*
         * Basic failure reporting
         */
        if (!result.success)
        {
            if (result.signal != 0)
            {
                printf("LiteShell: process terminated by signal %d\n",
                       result.signal);
            }
            else if (result.exit_code != -1)
            {
                printf("LiteShell: command failed (exit code %d)\n",
                       result.exit_code);
            }
        }
    }

    printf("\nLeaving LiteShell.\n");

    return 0;
}
