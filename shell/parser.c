#include <stdio.h>
#include <string.h>

#include "parser.h"

int parse_command(char *input, Command *command)
{
    command->argc = 0;

    char *token = strtok(input, " ");

    while (token != NULL && command->argc < MAX_ARGS - 1)
    {
        command->args[command->argc] = token;

        command->argc++;

        token = strtok(NULL, " ");
    }

    command->args[command->argc] = NULL;

    return command->argc;
}
