#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <sys/types.h>
#include "parser.h"

#define MAX_COMMAND_NAME 128
#define MAX_ERROR_MESSAGE 256
#define MAX_STDERR_OUTPUT 1024
#define MAX_RAW_ARGS 512

typedef struct
{
    int op_id;                          /* Sequential operation ID */
    char command[MAX_COMMAND_NAME];     /* Primary command executable name */
    char raw_args[MAX_RAW_ARGS];         /* Full argument string */
    
    int success;                        /* 1 if succeeded (exit 0), 0 otherwise */
    int exit_code;                      /* Process exit code (-1 if not set) */
    int signal;                         /* Termination signal if killed */
    int command_not_found;              /* 1 if execvp failed to find binary */
    
    int is_native;                      /* 1 if executed by native LiteShell handler */
    int errno_val;                      /* System errno for native operations */
    pid_t pid;                          /* Process ID of child process */
    
    char stderr_output[MAX_STDERR_OUTPUT]; /* Captured standard error output */
    char error_message[MAX_ERROR_MESSAGE]; /* Summary error string */
} OperationResult;

OperationResult execute_command(Command *command);

#endif

