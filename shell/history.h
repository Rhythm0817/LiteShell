#ifndef HISTORY_H
#define HISTORY_H

#include "executor.h"

#define MAX_HISTORY 100

void history_init(void);
int history_add(const OperationResult *op);
void history_print(void);
const OperationResult *history_get_by_id(int id);
const OperationResult *history_get_latest(void);
int history_count(void);

#endif
