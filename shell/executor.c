#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "executor.h"

OperationResult execute_command(Command *command)
{
    OperationResult result;

    result.success = 0;
    result.exit_code = -1;
    result.signal = 0;

    if (command->argc == 0)
    {
        return result;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("LiteShell: fork failed");
        return result;
    }

    /*
     * Child process
     */
    if (pid == 0)
    {
        execvp(command->args[0], command->args);

        /*
         * If execvp returns, the command could not be executed.
         */
        perror("LiteShell");
        exit(127);
    }

    /*
     * Parent process
     */
    int status;

    if (waitpid(pid, &status, 0) < 0)
    {
        perror("LiteShell: waitpid failed");
        return result;
    }

    /*
     * Normal process termination
     */
    if (WIFEXITED(status))
    {
        result.exit_code = WEXITSTATUS(status);

        if (result.exit_code == 0)
        {
            result.success = 1;
        }
    }

    /*
     * Process terminated by signal
     */
    else if (WIFSIGNALED(status))
    {
        result.signal = WTERMSIG(status);
    }

    return result;
}
