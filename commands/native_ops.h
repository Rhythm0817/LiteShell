#ifndef NATIVE_OPS_H
#define NATIVE_OPS_H

#include "../shell/parser.h"
#include "../shell/executor.h"

int is_native_command(const char *cmd_name);
OperationResult execute_native_command(Command *command);

#endif
