#ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H

#include "../shell/parser.h"
#include "../shell/executor.h"

typedef enum
{
    CMD_CAT_FILE,
    CMD_CAT_DIRECTORY,
    CMD_CAT_NAVIGATION,
    CMD_CAT_SYSTEM,
    CMD_CAT_DIAGNOSTICS,
    CMD_CAT_CONTROL
} CommandCategory;

typedef OperationResult (*CommandHandlerFunc)(Command *cmd);

typedef struct
{
    const char *name;
    const char *alias;
    const char *description;
    const char *usage;
    CommandCategory category;
    CommandHandlerFunc handler;
    int is_builtin_shell;
} CommandEntry;

void command_registry_init(void);
const CommandEntry *command_registry_lookup(const char *name);
void command_registry_print_help(void);
void command_registry_print_describe(const char *cmd_name);

#endif
