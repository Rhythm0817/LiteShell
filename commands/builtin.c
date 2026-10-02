#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/utsname.h>

#include "builtin.h"
#include "../core/command_registry.h"
#include "../shell/history.h"
#include "../ui/terminal.h"

static void print_about(void)
{
    struct utsname uts;
    char sysname[64] = "Linux";
    char machine[64] = "x86_64";
    if (uname(&uts) == 0)
    {
        strncpy(sysname, uts.sysname, sizeof(sysname) - 1);
        strncpy(machine, uts.machine, sizeof(machine) - 1);
    }

    printf("\nLiteShell OS\n");
    printf("Version 0.4\n\n");
    printf("A minimalist Linux-based command environment\n");
    printf("featuring LiteShell native command vocabulary,\n");
    printf("responsive terminal CUI layout, observation monitoring,\n");
    printf("and progressive diagnostic reporting.\n\n");
    printf("Platform    : %s\n", sysname);
    printf("Architecture: %s\n", machine);
    printf("Shell       : LiteShell Command Engine\n\n");
}

static void print_time(void)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    printf("Time : %s\n", time_str);
}

static void print_date(void)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%d-%m-%Y", tm_info);
    printf("Date : %s\n", date_str);
}

BuiltinResult handle_builtin(
    char *input,
    const OperationResult *last_operation,
    int *should_exit,
    DiagnosticLevel *out_level,
    int *out_target_id
)
{
    if (input == NULL) return BUILTIN_NOT_BUILTIN;

    char copy_input[MAX_INPUT_LINE];
    strncpy(copy_input, input, sizeof(copy_input) - 1);
    copy_input[sizeof(copy_input) - 1] = '\0';

    char *argv[16];
    int argc = 0;
    char *token = strtok(copy_input, " ");
    while (token != NULL && argc < 16)
    {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc == 0) return BUILTIN_NOT_BUILTIN;

    /* EXIT / QUIT */
    if (strcasecmp(argv[0], "exit") == 0 || strcasecmp(argv[0], "quit") == 0)
    {
        *should_exit = 1;
        return BUILTIN_EXIT;
    }

    /* CLEAR / CLS */
    if (strcasecmp(argv[0], "clear") == 0 || strcasecmp(argv[0], "cls") == 0)
    {
        terminal_clear_screen();
        return BUILTIN_HANDLED;
    }

    /* HELP */
    if (strcasecmp(argv[0], "help") == 0)
    {
        command_registry_print_help();
        return BUILTIN_HANDLED;
    }

    /* DESCRIBE */
    if (strcasecmp(argv[0], "describe") == 0)
    {
        const char *target_cmd = (argc > 1) ? argv[1] : "";
        command_registry_print_describe(target_cmd);
        return BUILTIN_HANDLED;
    }

    /* ABOUT */
    if (strcasecmp(argv[0], "about") == 0)
    {
        print_about();
        return BUILTIN_HANDLED;
    }

    /* TIME */
    if (strcasecmp(argv[0], "time") == 0)
    {
        print_time();
        return BUILTIN_HANDLED;
    }

    /* DATE */
    if (strcasecmp(argv[0], "date") == 0)
    {
        print_date();
        return BUILTIN_HANDLED;
    }

    /* HISTORY */
    if (strcasecmp(argv[0], "history") == 0)
    {
        history_print();
        return BUILTIN_HANDLED;
    }

    /* STATUS */
    if (strcasecmp(argv[0], "status") == 0)
    {
        const OperationResult *target_op = last_operation;
        if (argc > 1)
        {
            int req_id = atoi(argv[1]);
            const OperationResult *found = history_get_by_id(req_id);
            if (found != NULL)
            {
                target_op = found;
            }
            else
            {
                printf("Operation #%d not found in history.\n", req_id);
                return BUILTIN_HANDLED;
            }
        }

        if (target_op == NULL || target_op->command[0] == '\0')
        {
            printf("No operation has been executed yet.\n");
            return BUILTIN_HANDLED;
        }

        printf("Operation Status (#%d)\n", target_op->op_id);
        printf("---------------------\n");
        printf("Command   : %s\n", target_op->raw_args[0] ? target_op->raw_args : target_op->command);
        printf("Result    : %s\n", target_op->success ? "SUCCESS" : "FAILED");
        printf("Exit Code : %d\n", target_op->exit_code);
        if (target_op->signal != 0) printf("Signal    : %d\n", target_op->signal);
        if (target_op->errno_val != 0) printf("Errno     : %d (%s)\n", target_op->errno_val, strerror(target_op->errno_val));
        if (target_op->error_message[0] != '\0') printf("Message   : %s\n", target_op->error_message);
        printf("\n");

        return BUILTIN_HANDLED;
    }

    /* DIAGNOSE */
    if (strcasecmp(argv[0], "diagnose") == 0)
    {
        *out_level = DIAG_LEVEL_SUMMARY;
        *out_target_id = (argc > 1) ? atoi(argv[1]) : -1;
        return BUILTIN_DIAGNOSE;
    }

    /* INSPECT */
    if (strcasecmp(argv[0], "inspect") == 0)
    {
        *out_level = DIAG_LEVEL_CONTEXT;
        *out_target_id = -1;

        for (int i = 1; i < argc; i++)
        {
            if (strcasecmp(argv[i], "--deep") == 0 || strcasecmp(argv[i], "-d") == 0)
            {
                *out_level = DIAG_LEVEL_DEEP;
            }
            else if (strcasecmp(argv[i], "--system") == 0 || strcasecmp(argv[i], "-s") == 0)
            {
                *out_level = DIAG_LEVEL_OBSERVATION;
            }
            else if (strcasecmp(argv[i], "--context") == 0 || strcasecmp(argv[i], "-c") == 0)
            {
                *out_level = DIAG_LEVEL_CONTEXT;
            }
            else if (argv[i][0] != '-')
            {
                int id = atoi(argv[i]);
                if (id > 0)
                {
                    *out_target_id = id;
                }
            }
        }

        return BUILTIN_INSPECT;
    }

    return BUILTIN_NOT_BUILTIN;
}
