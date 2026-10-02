#ifndef BUILTIN_H
#define BUILTIN_H

#include "../shell/executor.h"
#include "../diagnostics/diagnostic.h"

typedef enum
{
    BUILTIN_NOT_BUILTIN = 0,
    BUILTIN_HANDLED = 1,
    BUILTIN_DIAGNOSE = 2,
    BUILTIN_INSPECT = 3,
    BUILTIN_EXIT = 4
} BuiltinResult;

BuiltinResult handle_builtin(
    char *input,
    const OperationResult *last_operation,
    int *should_exit,
    DiagnosticLevel *out_level,
    int *out_target_id
);

#endif
