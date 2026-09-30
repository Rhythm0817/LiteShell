#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

#define MAX_COMMAND_NAME 128
#define MAX_ERROR_MESSAGE 256

typedef struct
{
    char command[MAX_COMMAND_NAME];

    int success;

    int exit_code;

    int signal;

    int command_not_found;

    char error_message[MAX_ERROR_MESSAGE];

} OperationResult;

OperationResult execute_command(Command *command);

#endif
