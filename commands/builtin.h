#ifndef BUILTIN_H
#define BUILTIN_H

#include "../shell/executor.h"

int handle_builtin(
    char *input,
    OperationResult *last_operation,
    int *should_exit
);

#endif
