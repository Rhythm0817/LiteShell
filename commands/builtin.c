#include <stdio.h>
#include <string.h>

#include "builtin.h"

int handle_builtin(char *command)
{
    if (strcmp(command, "help") == 0)
    {
        printf("\n");
        printf("LiteShell Commands\n");
        printf("==================\n");
        printf("help       Show available commands\n");
        printf("exit       Exit LiteShell\n");
        printf("\n");

        printf("External Linux commands:\n");
        printf("ls         List files\n");
        printf("pwd        Show current directory\n");
        printf("whoami     Show current user\n");
        printf("echo       Display text\n");
        printf("date       Show system date and time\n");
        printf("\n");

        return 1;
    }

    return 0;
}
