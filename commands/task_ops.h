#ifndef TASK_OPS_H
#define TASK_OPS_H

#include "../shell/parser.h"
#include "../shell/executor.h"

OperationResult ops_task(Command *cmd);
OperationResult ops_task_info(Command *cmd);
OperationResult ops_task_stop(Command *cmd);

#endif
