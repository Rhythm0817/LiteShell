#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

typedef struct
{
    int success;
    int exit_code;
    int signal;
} OperationResult;

OperationResult execute_command(Command *command);

#endif
