#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

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
    char expanded_input[MAX_INPUT_LINE];
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
            if (terminal_check_interrupted() || errno == EINTR)
            {
                clearerr(stdin);
                printf("\n");
                continue;
            }
            break;
        }

        /* Remove trailing newlines */
        input[strcspn(input, "\r\n")] = '\0';

        /* Ignore blank lines */
        if (strlen(input) == 0)
        {
            continue;
        }

        /* Expand environment variables and exit codes ($VAR, $?, $$) */
        int last_code = 0;
        const OperationResult *last_op = history_get_latest();
        if (last_op != NULL)
        {
            last_code = last_op->exit_code;
        }
        expand_variables(input, expanded_input, sizeof(expanded_input), last_code);
        strncpy(input, expanded_input, sizeof(input) - 1);
        input[sizeof(input) - 1] = '\0';

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

        int saved_stdout = -1;
        int saved_stdin = -1;
        int redir_err = 0;

        /* Standard Input Redirection (<) */
        if (command.input_file != NULL)
        {
            int in_fd = open(command.input_file, O_RDONLY);
            if (in_fd < 0)
            {
                printf("LiteShell: cannot open '%s' for input: %s\n", command.input_file, strerror(errno));
                redir_err = 1;
            }
            else
            {
                saved_stdin = dup(STDIN_FILENO);
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
        }

        /* Standard Output Redirection (> or >>) */
        if (!redir_err && command.output_file != NULL)
        {
            int flags = O_WRONLY | O_CREAT | (command.append_output ? O_APPEND : O_TRUNC);
            int out_fd = open(command.output_file, flags, 0644);
            if (out_fd < 0)
            {
                printf("LiteShell: cannot open '%s' for output: %s\n", command.output_file, strerror(errno));
                redir_err = 1;
            }
            else
            {
                saved_stdout = dup(STDOUT_FILENO);
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
        }

        if (redir_err)
        {
            memset(&op_result, 0, sizeof(OperationResult));
            strncpy(op_result.command, command.args[0] ? command.args[0] : "redirection", sizeof(op_result.command) - 1);
            op_result.exit_code = 1;
            op_result.errno_val = errno;
            snprintf(op_result.stderr_output, sizeof(op_result.stderr_output), "Redirection failed: %s", strerror(errno));
            history_add(&op_result);
        }
        else
        {
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

        /* Restore stdin and stdout */
        if (saved_stdout >= 0)
        {
            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        if (saved_stdin >= 0)
        {
            dup2(saved_stdin, STDIN_FILENO);
            close(saved_stdin);
        }
    }

    return 0;
}
