#include <stdio.h>
#include <string.h>
#include "history.h"

static OperationResult history_ring[MAX_HISTORY];
static int history_total_count = 0;

void history_init(void)
{
    memset(history_ring, 0, sizeof(history_ring));
    history_total_count = 0;
}

int history_add(const OperationResult *op)
{
    if (op == NULL || op->command[0] == '\0')
    {
        return -1;
    }

    history_total_count++;
    int slot = (history_total_count - 1) % MAX_HISTORY;
    
    history_ring[slot] = *op;
    history_ring[slot].op_id = history_total_count;
    
    return history_total_count;
}

int history_count(void)
{
    return history_total_count;
}

const OperationResult *history_get_latest(void)
{
    if (history_total_count == 0)
    {
        return NULL;
    }
    int slot = (history_total_count - 1) % MAX_HISTORY;
    return &history_ring[slot];
}

const OperationResult *history_get_by_id(int id)
{
    if (id <= 0 || id > history_total_count)
    {
        return NULL;
    }
    
    int start_id = history_total_count > MAX_HISTORY ? (history_total_count - MAX_HISTORY + 1) : 1;
    if (id < start_id)
    {
        return NULL; /* Evicted from history window */
    }

    int slot = (id - 1) % MAX_HISTORY;
    return &history_ring[slot];
}

void history_print(void)
{
    if (history_total_count == 0)
    {
        printf("\nNo operation history available.\n\n");
        return;
    }

    int start_id = history_total_count > MAX_HISTORY ? (history_total_count - MAX_HISTORY + 1) : 1;

    printf("\n========================================\n");
    printf("         LiteShell Operation History\n");
    printf("========================================\n");
    printf("ID   STATUS    COMMAND / DETAILS\n");
    printf("----------------------------------------\n");

    for (int id = start_id; id <= history_total_count; id++)
    {
        int slot = (id - 1) % MAX_HISTORY;
        const OperationResult *op = &history_ring[slot];

        printf("#%-3d %-9s %s\n",
               op->op_id,
               op->success ? "SUCCESS" : "FAILED",
               op->raw_args[0] != '\0' ? op->raw_args : op->command);
        
        if (!op->success && op->error_message[0] != '\0')
        {
            printf("     └─ Reason: %s\n", op->error_message);
        }
    }
    printf("========================================\n\n");
}
