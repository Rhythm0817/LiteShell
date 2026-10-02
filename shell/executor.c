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
    result.is_native = 0;
    result.errno_val = 0;
    result.pid = -1;

    if (command->argc == 0)
    {
        return result;
    }

    /*
     * Store the command name and format raw argument string.
     */
    strncpy(
        result.command,
        command->args[0],
        MAX_COMMAND_NAME - 1
    );
    result.command[MAX_COMMAND_NAME - 1] = '\0';

    result.raw_args[0] = '\0';
    for (int i = 0; i < command->argc; i++)
    {
        if (i > 0)
        {
            strncat(result.raw_args, " ", sizeof(result.raw_args) - strlen(result.raw_args) - 1);
        }
        strncat(result.raw_args, command->args[i], sizeof(result.raw_args) - strlen(result.raw_args) - 1);
    }

    /*
     * Create pipe to capture stderr from child process
     */
    int stderr_pipe[2];
    if (pipe(stderr_pipe) < 0)
    {
        snprintf(
            result.error_message,
            MAX_ERROR_MESSAGE,
            "Unable to create internal pipe for diagnostic capture."
        );
        return result;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        snprintf(
            result.error_message,
            MAX_ERROR_MESSAGE,
            "Unable to create child process."
        );
        return result;
    }

    /*
     * Child process execution
     */
    if (pid == 0)
    {
        /* Close unused read end of stderr pipe */
        close(stderr_pipe[0]);

        /* Redirect stderr to pipe write-end */
        if (dup2(stderr_pipe[1], STDERR_FILENO) < 0)
        {
            perror("dup2 stderr redirection failed");
            close(stderr_pipe[1]);
            exit(127);
        }

        /* Close write end copy after dup2 */
        close(stderr_pipe[1]);

        execvp(command->args[0], command->args);

        /*
         * execvp failed. Exit code 127 indicates command not found/executable.
         */
        fprintf(stderr, "LiteShell: execvp failed for '%s'\n", command->args[0]);
        exit(127);
    }

    /*
     * Parent process execution
     */
    result.pid = pid;

    /*
     * CRITICAL: Close write end in parent so read() encounters EOF
     * when child process exits or closes write end.
     */
    close(stderr_pipe[1]);

    /* Read captured stderr output from child */
    ssize_t total_bytes = 0;
    ssize_t bytes_read = 0;
    while (total_bytes < (ssize_t)(sizeof(result.stderr_output) - 1))
    {
        bytes_read = read(stderr_pipe[0], result.stderr_output + total_bytes, sizeof(result.stderr_output) - 1 - total_bytes);
        if (bytes_read <= 0)
        {
            break;
        }
        total_bytes += bytes_read;
    }
    result.stderr_output[total_bytes] = '\0';
    close(stderr_pipe[0]);

    /* Clean trailing newlines from stderr output buffer */
    while (total_bytes > 0 && (result.stderr_output[total_bytes - 1] == '\n' || result.stderr_output[total_bytes - 1] == '\r'))
    {
        result.stderr_output[--total_bytes] = '\0';
    }

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
     * Normal termination evaluation
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
            if (result.stderr_output[0] != '\0')
            {
                snprintf(
                    result.error_message,
                    MAX_ERROR_MESSAGE,
                    "%.250s",
                    result.stderr_output
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
    }
    /*
     * Terminated by signal evaluation
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

