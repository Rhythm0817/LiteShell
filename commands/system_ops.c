#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/utsname.h>

#include "system_ops.h"
#include "../ui/terminal.h"

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

OperationResult ops_sysinfo(Command *cmd)
{
    OperationResult res;
    init_result(&res, "sysinfo", cmd);

    struct utsname uts;
    if (uname(&uts) != 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "uname() failed: %s", strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Failed to read kernel information.");
        return res;
    }

    char hostname[256] = "Unknown";
    gethostname(hostname, sizeof(hostname));

    double uptime_secs = 0;
    FILE *f_up = fopen("/proc/uptime", "r");
    if (f_up != NULL)
    {
        fscanf(f_up, "%lf", &uptime_secs);
        fclose(f_up);
    }

    long hours = (long)uptime_secs / 3600;
    long mins = ((long)uptime_secs % 3600) / 60;

    char cpu_model[256] = "Generic x86_64";
    FILE *f_cpu = fopen("/proc/cpuinfo", "r");
    if (f_cpu != NULL)
    {
        char line[256];
        while (fgets(line, sizeof(line), f_cpu))
        {
            if (strncmp(line, "model name", 10) == 0)
            {
                char *colon = strchr(line, ':');
                if (colon != NULL)
                {
                    strncpy(cpu_model, colon + 2, sizeof(cpu_model) - 1);
                    cpu_model[sizeof(cpu_model) - 1] = '\0';
                    cpu_model[strcspn(cpu_model, "\r\n")] = '\0';
                    break;
                }
            }
        }
        fclose(f_cpu);
    }

    printf("\nSYSTEM INFORMATION\n");
    printf("------------------\n");
    printf("Environment  : LiteShell OS v0.4 (Linux Environment)\n");
    printf("Kernel       : %s %s\n", uts.sysname, uts.release);
    printf("Hostname     : %s\n", hostname);
    printf("Architecture : %s\n", uts.machine);
    printf("Processor    : %s\n", cpu_model);
    printf("System Uptime: %ld hours %ld mins\n\n", hours, mins);

    res.success = 1;
    res.exit_code = 0;
    return res;
}

OperationResult ops_mem(Command *cmd)
{
    OperationResult res;
    init_result(&res, "mem", cmd);

    FILE *f = fopen("/proc/meminfo", "r");
    if (f == NULL)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "fopen('/proc/meminfo') failed: %s", strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Failed to access system memory statistics.");
        return res;
    }

    long mem_total = 0, mem_free = 0, mem_available = 0, swap_total = 0, swap_free = 0;
    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "MemTotal:", 9) == 0) sscanf(line, "MemTotal:\t%ld", &mem_total);
        else if (strncmp(line, "MemFree:", 8) == 0) sscanf(line, "MemFree:\t%ld", &mem_free);
        else if (strncmp(line, "MemAvailable:", 13) == 0) sscanf(line, "MemAvailable:\t%ld", &mem_available);
        else if (strncmp(line, "SwapTotal:", 10) == 0) sscanf(line, "SwapTotal:\t%ld", &swap_total);
        else if (strncmp(line, "SwapFree:", 9) == 0) sscanf(line, "SwapFree:\t%ld", &swap_free);
    }
    fclose(f);

    long mem_used = mem_total - mem_available;

    printf("\nMEMORY INFORMATION\n");
    printf("------------------\n");
    printf("Total Memory : %8ld MB\n", mem_total / 1024);
    printf("Used Memory  : %8ld MB\n", mem_used / 1024);
    printf("Free Memory  : %8ld MB\n", mem_free / 1024);
    printf("Available    : %8ld MB\n", mem_available / 1024);
    if (swap_total > 0)
    {
        printf("Swap Total   : %8ld MB\n", swap_total / 1024);
        printf("Swap Free    : %8ld MB\n", swap_free / 1024);
    }
    printf("\n");

    res.success = 1;
    res.exit_code = 0;
    return res;
}

static int is_numeric_dir(const char *name)
{
    if (name == NULL || name[0] == '\0') return 0;
    for (int i = 0; name[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)name[i])) return 0;
    }
    return 1;
}

OperationResult ops_proc(Command *cmd)
{
    OperationResult res;
    init_result(&res, "proc", cmd);

    DIR *dir = opendir("/proc");
    if (dir == NULL)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "opendir('/proc') failed: %s", strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot open /proc directory.");
        return res;
    }

    int cols = terminal_get_cols();
    if (cols >= 80)
    {
        printf("\n%-8s %-24s %-12s %-10s %-8s\n", "PID", "NAME", "STATE", "UID", "THREADS");
        printf("--------------------------------------------------------------------------------\n");
    }
    else
    {
        printf("\n%-8s %-20s %-12s\n", "PID", "NAME", "STATE");
        printf("--------------------------------------------------\n");
    }

    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (is_numeric_dir(entry->d_name))
        {
            int pid = atoi(entry->d_name);
            char status_path[256];
            snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);

            FILE *f = fopen(status_path, "r");
            if (f != NULL)
            {
                char name[64] = "Unknown";
                char state[32] = "Unknown";
                unsigned int uid = 0;
                int threads = 1;
                char line[256];

                while (fgets(line, sizeof(line), f))
                {
                    if (strncmp(line, "Name:", 5) == 0)
                    {
                        sscanf(line, "Name:\t%63s", name);
                    }
                    else if (strncmp(line, "State:", 6) == 0)
                    {
                        sscanf(line, "State:\t%31s", state);
                    }
                    else if (strncmp(line, "Uid:", 4) == 0)
                    {
                        sscanf(line, "Uid:\t%u", &uid);
                    }
                    else if (strncmp(line, "Threads:", 8) == 0)
                    {
                        sscanf(line, "Threads:\t%d", &threads);
                    }
                }
                fclose(f);

                if (cols >= 80)
                {
                    printf("%-8d %-24.24s %-12s %-10u %-8d\n", pid, name, state, uid, threads);
                }
                else
                {
                    printf("%-8d %-20.20s %-12s\n", pid, name, state);
                }
                count++;

                if (count >= 25 && cmd->argc == 1)
                {
                    printf("... (%d+ additional processes running, use 'proc --all' to list all)\n", count);
                    break;
                }
            }
        }
    }
    closedir(dir);

    if (cols >= 80)
    {
        printf("--------------------------------------------------------------------------------\n");
    }
    else
    {
        printf("--------------------------------------------------\n");
    }
    printf("Total listed processes: %d\n\n", count);

    res.success = 1;
    res.exit_code = 0;
    return res;
}
