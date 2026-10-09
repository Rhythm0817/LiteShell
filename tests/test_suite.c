#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "../shell/parser.h"
#include "../shell/executor.h"
#include "../shell/history.h"
#include "../core/command_registry.h"
#include "../commands/filesystem_ops.h"
#include "../commands/system_ops.h"
#include "../commands/task_ops.h"
#include "../commands/monitor_ops.h"
#include "../diagnostics/diagnostic.h"
#include "../ui/terminal.h"

typedef struct
{
    const char *name;
    const char *command_str;
    DiagnosticType expected_type;
} TestCase;

static double get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000.0) + (ts.tv_nsec / 1000000.0);
}

int main(void)
{
    history_init();
    command_registry_init();
    terminal_init();

    /* Pre-clean test paths if left over from previous runs */
    unlink("/tmp/liteshell_test_file.txt");
    unlink("/tmp/liteshell_test_copy.txt");
    unlink("/tmp/liteshell_test_moved.txt");
    unlink("/tmp/liteshell space test.txt");
    unlink("/tmp/liteshell_test_redir.txt");
    rmdir("/tmp/liteshell_test_dir");

    TestCase tests[] = {
        {"1. Native Curdir (pwd)",         "curdir",                                            DIAG_SUCCESS},
        {"2. Native System Info",         "sysinfo",                                           DIAG_SUCCESS},
        {"3. Native Memory Stats",        "mem",                                               DIAG_SUCCESS},
        {"4. Native Directory List",      "dir .",                                             DIAG_SUCCESS},
        {"5. Native Task Process List",   "proc",                                              DIAG_SUCCESS},
        {"6. Native Task Info",           "taskinfo",                                          DIAG_SUCCESS},
        {"7. Native Make File",           "makef /tmp/liteshell_test_file.txt",                DIAG_SUCCESS},
        {"8. Native Type File",           "typef /tmp/liteshell_test_file.txt",                DIAG_SUCCESS},
        {"9. Native Copy File",           "copyf /tmp/liteshell_test_file.txt /tmp/liteshell_test_copy.txt", DIAG_SUCCESS},
        {"10. Native Move File",          "movef /tmp/liteshell_test_copy.txt /tmp/liteshell_test_moved.txt", DIAG_SUCCESS},
        {"11. Native Delete File",        "delf /tmp/liteshell_test_file.txt",                 DIAG_SUCCESS},
        {"12. Native Make Directory",     "makedir /tmp/liteshell_test_dir",                   DIAG_SUCCESS},
        {"13. Native Change Directory",   "cdir /tmp/liteshell_test_dir",                      DIAG_SUCCESS},
        {"14. Native Delete Directory",   "deldir /tmp/liteshell_test_dir",                    DIAG_SUCCESS},
        {"15. System Monitor Overview",    "monitor",                                           DIAG_SUCCESS},
        {"16. System Monitor Process View","monitor proc",                                      DIAG_SUCCESS},
        {"17. Command Not Found",         "nonexistent_cmd_xyz987",                             DIAG_COMMAND_NOT_FOUND},
        {"18. Native File Not Found",     "typef /tmp/liteshell_nonexistent_file_987.txt",     DIAG_FILE_NOT_FOUND},
        {"19. Native Dir Not Found",      "dir /tmp/nonexistent_dir_abc_987/test.txt",         DIAG_DIRECTORY_NOT_FOUND},
        {"20. Native Copy Missing Src",   "copyf /tmp/missing_src_xyz.txt /tmp/dst.txt",       DIAG_FILE_NOT_FOUND},
        {"21. Native Copy Missing Dir",   "copyf /etc/hosts /tmp/nonexistent_dir_xyz/dst.txt", DIAG_DIRECTORY_NOT_FOUND},
        {"22. Native Mkdir Already Exists","makedir /tmp",                                     DIAG_INVALID_ARGUMENT},
        {"23. Permission Failure",        "typef /etc/shadow",                                 DIAG_PERMISSION_DENIED},
        {"24. Quoted Path Makef",         "makef \"/tmp/liteshell space test.txt\"",           DIAG_SUCCESS},
        {"25. Quoted Path Typef",         "typef \"/tmp/liteshell space test.txt\"",           DIAG_SUCCESS},
        {"26. Redirection Out (>)",       "dir > /tmp/liteshell_test_redir.txt",               DIAG_SUCCESS},
        {"27. Redirection Append (>>)",   "mem >> /tmp/liteshell_test_redir.txt",              DIAG_SUCCESS},
        {"28. Quoted Path Delf",          "delf \"/tmp/liteshell space test.txt\"",            DIAG_SUCCESS}
    };

    int total_tests = sizeof(tests) / sizeof(TestCase);
    int passed = 0;

    printf("========================================================================================\n");
    printf("         LiteShell OS - Native Command & Responsive CUI Evaluation Suite\n");
    printf("========================================================================================\n\n");

    printf("%-30s | %-20s | %-20s | %-10s | %-8s | %-7s\n",
           "Test Case", "Expected Diagnosis", "Actual Diagnosis", "Confidence", "Time(ms)", "Result");
    printf("----------------------------------------------------------------------------------------\n");

    for (int i = 0; i < total_tests; i++)
    {
        char input_buf[MAX_INPUT_LINE];
        strncpy(input_buf, tests[i].command_str, sizeof(input_buf) - 1);
        input_buf[sizeof(input_buf) - 1] = '\0';

        Command cmd;
        parse_command(input_buf, &cmd);

        double t0 = get_time_ms();

        const CommandEntry *entry = command_registry_lookup(cmd.args[0]);
        OperationResult op;

        int saved_stdout = -1;
        int saved_stdin = -1;

        if (cmd.output_file != NULL)
        {
            int flags = O_WRONLY | O_CREAT | (cmd.append_output ? O_APPEND : O_TRUNC);
            int out_fd = open(cmd.output_file, flags, 0644);
            if (out_fd >= 0)
            {
                saved_stdout = dup(STDOUT_FILENO);
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
        }

        if (cmd.input_file != NULL)
        {
            int in_fd = open(cmd.input_file, O_RDONLY);
            if (in_fd >= 0)
            {
                saved_stdin = dup(STDIN_FILENO);
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
        }

        if (entry != NULL && entry->handler != NULL)
        {
            op = entry->handler(&cmd);
        }
        else
        {
            op = execute_command(&cmd);
        }

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

        double t1 = get_time_ms();
        double duration = t1 - t0;

        history_add(&op);
        DiagnosticResult diag = diagnose_operation(&op, DIAG_LEVEL_OBSERVATION);

        int is_correct = (diag.type == tests[i].expected_type);
        if (is_correct) passed++;

        printf("%-30s | %-20s | %-20s | %-10s | %8.2f | %s\n",
           tests[i].name,
           diagnostic_type_str(tests[i].expected_type),
           diagnostic_type_str(diag.type),
           diagnostic_confidence_str(diag.confidence),
           duration,
           is_correct ? "\033[32mPASS\033[0m" : "\033[31mFAIL\033[0m");
    }

    /* Cleanup temporary test files */
    unlink("/tmp/liteshell_test_file.txt");
    unlink("/tmp/liteshell_test_copy.txt");
    unlink("/tmp/liteshell_test_moved.txt");
    unlink("/tmp/liteshell space test.txt");
    unlink("/tmp/liteshell_test_redir.txt");
    rmdir("/tmp/liteshell_test_dir");

    printf("----------------------------------------------------------------------------------------\n");
    printf("Evaluation Summary: %d / %d Tests Passed (%.1f%% Accuracy)\n",
           passed, total_tests, (double)passed / total_tests * 100.0);
    printf("========================================================================================\n\n");

    return (passed == total_tests) ? 0 : 1;
}
