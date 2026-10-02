#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "task_ops.h"
#include "system_ops.h"

static void init_result(OperationResult *res, const char *cmd_name, Command *cmd)
{
    memset(res, 0, sizeof(OperationResult));
    res->is_native = 1;
    res->pid = getpid();
    res->exit_code = -1;
    strncpy(res->command, cmd_name, sizeof(res->command) - 1);

    res->raw_args[0] = '\0';
    for (int i = 0; i < cmd->argc; i++)
    {
        if (i > 0)
        {
            strncat(res->raw_args, " ", sizeof(res->raw_args) - strlen(res->raw_args) - 1);
        }
        strncat(res->raw_args, cmd->args[i], sizeof(res->raw_args) - strlen(res->raw_args) - 1);
    }
}

OperationResult ops_task_info(Command *cmd)
{
    OperationResult res;
    init_result(&res, "taskinfo", cmd);

    int target_pid = -1;
    for (int i = 1; i < cmd->argc; i++)
    {
        if (strcmp(cmd->args[i], "info") != 0 && cmd->args[i][0] != '-')
        {
            target_pid = atoi(cmd->args[i]);
            break;
        }
    }

    if (target_pid <= 0)
    {
        target_pid = getpid(); /* Default to current LiteShell process */
    }

    char status_path[256];
    snprintf(status_path, sizeof(status_path), "/proc/%d/status", target_pid);

    FILE *f = fopen(status_path, "r");
    if (f == NULL)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "fopen('%.200s') failed: %s", status_path, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Task PID %d does not exist or access was denied.", target_pid);
        return res;
    }

    char name[128] = "Unknown";
    char state[64] = "Unknown";
    int ppid = 0, uid = 0, gid = 0, threads = 1;
    long vm_size = 0, vm_rss = 0;
    char line[256];

    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "Name:", 5) == 0) sscanf(line, "Name:\t%127s", name);
        else if (strncmp(line, "State:", 6) == 0) sscanf(line, "State:\t%63[^\n]", state);
        else if (strncmp(line, "PPid:", 5) == 0) sscanf(line, "PPid:\t%d", &ppid);
        else if (strncmp(line, "Uid:", 4) == 0) sscanf(line, "Uid:\t%d", &uid);
        else if (strncmp(line, "Gid:", 4) == 0) sscanf(line, "Gid:\t%d", &gid);
        else if (strncmp(line, "Threads:", 8) == 0) sscanf(line, "Threads:\t%d", &threads);
        else if (strncmp(line, "VmSize:", 7) == 0) sscanf(line, "VmSize:\t%ld", &vm_size);
        else if (strncmp(line, "VmRSS:", 6) == 0) sscanf(line, "VmRSS:\t%ld", &vm_rss);
    }
    fclose(f);

    /* Read Working Directory from /proc/<pid>/cwd */
    char cwd[256] = "Unknown";
    char cwd_link[256];
    snprintf(cwd_link, sizeof(cwd_link), "/proc/%d/cwd", target_pid);
    ssize_t cwd_len = readlink(cwd_link, cwd, sizeof(cwd) - 1);
    if (cwd_len > 0) cwd[cwd_len] = '\0';

    /* Read Executable Path from /proc/<pid>/cmdline */
    char cmdline[512] = "Unknown";
    char cmd_link[256];
    snprintf(cmd_link, sizeof(cmd_link), "/proc/%d/cmdline", target_pid);
    FILE *f_cmd = fopen(cmd_link, "r");
    if (f_cmd != NULL)
    {
        size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, f_cmd);
        fclose(f_cmd);
        if (n > 0)
        {
            cmdline[n] = '\0';
            for (size_t i = 0; i < n - 1; i++)
            {
                if (cmdline[i] == '\0') cmdline[i] = ' ';
            }
        }
    }

    printf("\nTASK INFORMATION (#%d)\n", target_pid);
    printf("----------------------------------------\n");
    printf("PID          : %d\n", target_pid);
    printf("Name         : %s\n", name);
    printf("State        : %s\n", state);
    printf("Command      : %s\n", cmdline[0] ? cmdline : name);
    printf("Working Dir  : %s\n", cwd);
    printf("Parent PID   : %d\n", ppid);
    printf("User UID/GID : %d / %d\n", uid, gid);
    printf("Threads      : %d\n", threads);
    if (vm_rss > 0) printf("Memory (RSS) : %ld KB\n", vm_rss);
    if (vm_size > 0) printf("Virtual Size : %ld KB\n", vm_size);
    printf("\n");

    res.success = 1;
    res.exit_code = 0;
    return res;
}

OperationResult ops_task_stop(Command *cmd)
{
    OperationResult res;
    init_result(&res, "taskstop", cmd);

    int target_pid = -1;
    int sig = SIGTERM;

    for (int i = 1; i < cmd->argc; i++)
    {
        if (strcmp(cmd->args[i], "-9") == 0 || strcmp(cmd->args[i], "--force") == 0)
        {
            sig = SIGKILL;
        }
        else if (strcmp(cmd->args[i], "stop") != 0 && strcmp(cmd->args[i], "kill") != 0 && cmd->args[i][0] != '-')
        {
            target_pid = atoi(cmd->args[i]);
        }
    }

    if (target_pid <= 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: task stop <pid> [--force]");
        strcpy(res.error_message, "Missing valid target process ID.");
        return res;
    }

    if (kill(target_pid, sig) != 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "kill(%d, %d) failed: %s", target_pid, sig, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Failed to stop task #%d: %s", target_pid, strerror(errno));
        return res;
    }

    printf("Task #%d terminated successfully.\n", target_pid);
    res.success = 1;
    res.exit_code = 0;
    return res;
}

OperationResult ops_task(Command *cmd)
{
    if (cmd->argc > 1)
    {
        if (strcmp(cmd->args[1], "info") == 0)
        {
            return ops_task_info(cmd);
        }
        if (strcmp(cmd->args[1], "stop") == 0 || strcmp(cmd->args[1], "kill") == 0)
        {
            return ops_task_stop(cmd);
        }
    }
    return ops_proc(cmd);
}
