#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "executor.h"

OperationResult execute_command(Command *command)
{
    OperationResult result;

    memset(&result, 0, sizeof(OperationResult));

    result.success = 0;
    result.exit_code = -1;
    result.signal = 0;
    result.command_not_found = 0;

    if (command->argc == 0)
    {
        return result;
    }

    /*
     * Store the command name.
     */
    strncpy(
        result.command,
        command->args[0],
        MAX_COMMAND_NAME - 1
    );

    result.command[MAX_COMMAND_NAME - 1] = '\0';

    pid_t pid = fork();

    if (pid < 0)
    {
        snprintf(
            result.error_message,
            MAX_ERROR_MESSAGE,
            "Unable to create child process."
        );

        return result;
    }

    /*
     * Child process
     */
    if (pid == 0)
    {
        execvp(command->args[0], command->args);

        /*
         * execvp failed.
         *
         * Exit code 127 is traditionally used to indicate
         * that a command could not be found/executed.
         */
        exit(127);
    }

    /*
     * Parent process
     */
    int status;

    if (waitpid(pid, &status, 0) < 0)
    {
        snprintf(
            result.error_message,
            MAX_ERROR_MESSAGE,
            "Unable to wait for the child process."
        );

        return result;
    }

    /*
     * Normal termination
     */
    if (WIFEXITED(status))
    {
        result.exit_code = WEXITSTATUS(status);

        if (result.exit_code == 0)
        {
            result.success = 1;
        }
        else if (result.exit_code == 127)
        {
            result.command_not_found = 1;

            snprintf(
                result.error_message,
                MAX_ERROR_MESSAGE,
                "Command '%s' could not be found.",
                result.command
            );
        }
        else
        {
            snprintf(
                result.error_message,
                MAX_ERROR_MESSAGE,
                "Command exited with status %d.",
                result.exit_code
            );
        }
    }

    /*
     * Terminated by signal
     */
    else if (WIFSIGNALED(status))
    {
        result.signal = WTERMSIG(status);

        snprintf(
            result.error_message,
            MAX_ERROR_MESSAGE,
            "Process terminated by signal %d.",
            result.signal
        );
    }

    return result;
}
