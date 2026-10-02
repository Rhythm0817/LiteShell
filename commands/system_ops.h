#ifndef SYSTEM_OPS_H
#define SYSTEM_OPS_H

#include "../shell/parser.h"
#include "../shell/executor.h"

OperationResult ops_sysinfo(Command *cmd);
OperationResult ops_mem(Command *cmd);
OperationResult ops_proc(Command *cmd);

#endif
