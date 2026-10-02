#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "command_registry.h"
#include "../commands/filesystem_ops.h"
#include "../commands/system_ops.h"
#include "../commands/task_ops.h"
#include "../commands/monitor_ops.h"

static CommandEntry registry[] = {
    /* FILE OPERATIONS */
    {"makef",   NULL,     "Create a file",                               "makef <file>",              CMD_CAT_FILE,       ops_makef,   0},
    {"delf",    "rm",     "Delete a file",                               "delf <file>",               CMD_CAT_FILE,       ops_delf,    0},
    {"copyf",   "cp",     "Copy a file",                                 "copyf <src> <dst>",         CMD_CAT_FILE,       ops_copyf,   0},
    {"movef",   "mv",     "Move a file",                                 "movef <src> <dst>",         CMD_CAT_FILE,       ops_movef,   0},
    {"typef",   "cat",    "Display file contents",                       "typef <file>",              CMD_CAT_FILE,       ops_typef,   0},

    /* DIRECTORY OPERATIONS */
    {"dir",     "ls",     "List directory contents",                     "dir [path]",                CMD_CAT_DIRECTORY,  ops_dir,     0},
    {"makedir", "mkdir",  "Create directory",                            "makedir <dir>",             CMD_CAT_DIRECTORY,  ops_makedir, 0},
    {"deldir",  "rmdir",  "Delete directory",                            "deldir <dir>",              CMD_CAT_DIRECTORY,  ops_deldir,  0},
    {"copydir", NULL,     "Copy directory",                              "copydir <src> <dst>",       CMD_CAT_DIRECTORY,  ops_copydir, 0},
    {"movedir", NULL,     "Move directory",                              "movedir <src> <dst>",       CMD_CAT_DIRECTORY,  ops_movedir, 0},

    /* NAVIGATION OPERATIONS */
    {"cdir",    "cd",     "Change directory",                            "cdir <path>",               CMD_CAT_NAVIGATION, ops_cdir,    0},
    {"curdir",  "pwd",    "Show current directory",                      "curdir",                    CMD_CAT_NAVIGATION, ops_curdir,  0},

    /* SYSTEM OPERATIONS */
    {"sysinfo", NULL,     "System information",                          "sysinfo",                   CMD_CAT_SYSTEM,     ops_sysinfo, 0},
    {"mem",     NULL,     "Memory information",                          "mem",                       CMD_CAT_SYSTEM,     ops_mem,     0},
    {"proc",    "task",   "Process information",                         "proc [--all]",              CMD_CAT_SYSTEM,     ops_proc,    0},
    {"taskinfo",NULL,     "Display process status & resource info",      "taskinfo <pid>",            CMD_CAT_SYSTEM,     ops_task_info,0},
    {"taskstop","kill",   "Terminate a process by PID",                  "taskstop <pid> [--force]",  CMD_CAT_SYSTEM,     ops_task_stop,0},
    {"monitor", "sysmon", "System monitor",                              "monitor [mem|cpu|sys|proc]",CMD_CAT_SYSTEM,     ops_monitor,  0},

    /* DIAGNOSTICS */
    {"status",  NULL,     "Last operation result",                       "status [id]",               CMD_CAT_DIAGNOSTICS,NULL,        1},
    {"diagnose",NULL,     "Explain failure cause",                       "diagnose [id]",             CMD_CAT_DIAGNOSTICS,NULL,        1},
    {"inspect", NULL,     "Technical inspection",                        "inspect [id] [--deep]",     CMD_CAT_DIAGNOSTICS,NULL,        1},

    /* GENERAL CONTROL */
    {"help",    NULL,     "Display command reference",                   "help",                      CMD_CAT_CONTROL,    NULL,        1},
    {"history", NULL,     "Command history",                             "history",                   CMD_CAT_CONTROL,    NULL,        1},
    {"clear",   "cls",    "Clear screen",                                "clear",                     CMD_CAT_CONTROL,    NULL,        1},
    {"date",    NULL,     "Display current date",                        "date",                      CMD_CAT_CONTROL,    NULL,        1},
    {"time",    NULL,     "Display current time",                        "time",                      CMD_CAT_CONTROL,    NULL,        1},
    {"about",   NULL,     "About LiteShell",                             "about",                     CMD_CAT_CONTROL,    NULL,        1},
    {"exit",    "quit",   "Exit LiteShell",                              "exit",                      CMD_CAT_CONTROL,    NULL,        1},

    /* Legacy compatibility entries */
    {"copy",    NULL,     "Copy file (legacy)",                          "copy <src> <dst>",          CMD_CAT_FILE,       ops_copyf,   0},
    {"move",    NULL,     "Move file (legacy)",                          "move <src> <dst>",          CMD_CAT_FILE,       ops_movef,   0},
    {"del",     NULL,     "Delete file (legacy)",                        "del <file>",                CMD_CAT_FILE,       ops_delf,    0},
    {"type",    NULL,     "Display file contents (legacy)",              "type <file>",               CMD_CAT_FILE,       ops_typef,   0}
};

static const int registry_size = sizeof(registry) / sizeof(CommandEntry);

void command_registry_init(void)
{
}

const CommandEntry *command_registry_lookup(const char *name)
{
    if (name == NULL || name[0] == '\0') return NULL;

    for (int i = 0; i < registry_size; i++)
    {
        if (strcmp(name, registry[i].name) == 0)
        {
            return &registry[i];
        }
        if (registry[i].alias != NULL && strcmp(name, registry[i].alias) == 0)
        {
            return &registry[i];
        }
    }
    return NULL;
}

static void print_category_commands(CommandCategory cat)
{
    for (int i = 0; i < registry_size; i++)
    {
        if (registry[i].category == cat)
        {
            /* Hide legacy alias entries from main HELP list */
            if (strcmp(registry[i].name, "copy") == 0 || strcmp(registry[i].name, "move") == 0 ||
                strcmp(registry[i].name, "del") == 0 || strcmp(registry[i].name, "type") == 0 ||
                strcmp(registry[i].name, "taskinfo") == 0 || strcmp(registry[i].name, "taskstop") == 0)
            {
                continue;
            }
            printf("  %-22s %s\n", registry[i].usage, registry[i].description);
        }
    }
}

void command_registry_print_help(void)
{
    printf("\nLITESHELL COMMAND REFERENCE\n");
    printf("============================\n\n");

    printf("FILE\n");
    print_category_commands(CMD_CAT_FILE);

    printf("\nDIRECTORY\n");
    print_category_commands(CMD_CAT_DIRECTORY);

    printf("\nNAVIGATION\n");
    print_category_commands(CMD_CAT_NAVIGATION);

    printf("\nSYSTEM\n");
    print_category_commands(CMD_CAT_SYSTEM);

    printf("\nDIAGNOSTICS\n");
    print_category_commands(CMD_CAT_DIAGNOSTICS);

    printf("\nCONTROL\n");
    print_category_commands(CMD_CAT_CONTROL);

    printf("\nALIASES FOR COMPATIBILITY:\n");
    printf("  cd -> cdir | pwd -> curdir | ls -> dir | cat -> typef\n");
    printf("  cp -> copyf | mv -> movef | rm -> delf | mkdir -> makedir | rmdir -> deldir\n\n");
    printf("External Linux commands (e.g., grep, whoami) execute via fallback executor.\n\n");
}

void command_registry_print_describe(const char *cmd_name)
{
    const CommandEntry *entry = command_registry_lookup(cmd_name);
    if (entry == NULL)
    {
        printf("\nNo native description for '%s'. External binary fallback.\n\n", cmd_name);
        return;
    }

    printf("\nCommand        : %s\n", entry->name);
    if (entry->alias)
    {
        printf("Alias          : %s\n", entry->alias);
    }
    printf("Category       : %s\n",
           entry->category == CMD_CAT_FILE ? "FILE" :
           entry->category == CMD_CAT_DIRECTORY ? "DIRECTORY" :
           entry->category == CMD_CAT_NAVIGATION ? "NAVIGATION" :
           entry->category == CMD_CAT_SYSTEM ? "SYSTEM" :
           entry->category == CMD_CAT_DIAGNOSTICS ? "DIAGNOSTICS" : "CONTROL");
    printf("Usage          : %s\n", entry->usage);
    printf("Description    : %s\n\n", entry->description);
}
