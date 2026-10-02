#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "parser.h"
#include "executor.h"
#include "history.h"
#include "../core/command_registry.h"
#include "../commands/builtin.h"
#include "../diagnostics/diagnostic.h"
#include "../observation/observation.h"
#include "../ui/terminal.h"
#include "../ui/screen.h"

int main(void)
{
    char input[MAX_INPUT_LINE];
    int should_exit = 0;

    history_init();
    command_registry_init();
    terminal_init();

    screen_render_header();
    printf("\nSystem ready. Type HELP for a list of native commands.\n\n");
    screen_render_footer();
    printf("\n");

    while (!should_exit)
    {
        if (terminal_check_resize())
        {
            printf("\n[Terminal resized: %dx%d]\n", terminal_get_cols(), terminal_get_rows());
        }

        screen_render_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }

        /* Remove trailing newlines */
        input[strcspn(input, "\r\n")] = '\0';

        /* Ignore blank lines */
        if (strlen(input) == 0)
        {
            continue;
        }

        DiagnosticLevel level = DIAG_LEVEL_SUMMARY;
        int target_id = -1;

        BuiltinResult builtin_res = handle_builtin(
            input,
            history_get_latest(),
            &should_exit,
            &level,
            &target_id
        );

        if (builtin_res == BUILTIN_EXIT)
        {
            break;
        }

        if (builtin_res == BUILTIN_HANDLED)
        {
            continue;
        }

        if (builtin_res == BUILTIN_DIAGNOSE || builtin_res == BUILTIN_INSPECT)
        {
            const OperationResult *target_op = NULL;

            if (target_id > 0)
            {
                target_op = history_get_by_id(target_id);
                if (target_op == NULL)
                {
                    printf("Operation #%d not found in history.\n\n", target_id);
                    continue;
                }
            }
            else
            {
                target_op = history_get_latest();
            }

            if (target_op == NULL || target_op->command[0] == '\0')
            {
                printf("No operation is available for inspection.\n\n");
                continue;
            }

            DiagnosticResult diag = diagnose_operation(target_op, level);
            print_diagnostic(&diag);
            continue;
        }

        /* Parse command string */
        Command command;
        if (parse_command(input, &command) == 0)
        {
            continue;
        }

        /* Check Command Registry for native handler */
        const CommandEntry *entry = command_registry_lookup(command.args[0]);
        OperationResult op_result;

        if (entry != NULL && entry->handler != NULL)
        {
            op_result = entry->handler(&command);
        }
        else
        {
            /* Fallback to external Linux binary execution */
            op_result = execute_command(&command);
        }

        history_add(&op_result);

        if (!op_result.success)
        {
            printf("\nOperation failed.\nUse 'diagnose' for details.\n\n");
        }
    }

    return 0;
}
