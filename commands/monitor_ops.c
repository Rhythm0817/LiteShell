#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "monitor_ops.h"
#include "../observation/observation.h"
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

static void render_system(void)
{
    SystemObservation sys = observe_system();
    long hours = (long)sys.uptime_secs / 3600;
    long mins = ((long)sys.uptime_secs % 3600) / 60;
    long secs = (long)sys.uptime_secs % 60;

    printf("\nSYSTEM\n");
    printf("----------------------------------------\n");
    printf("Host          : %s\n", sys.hostname[0] ? sys.hostname : "localhost");
    printf("Kernel        : %s %s\n", sys.sysname, sys.release);
    printf("Architecture  : %s\n", sys.machine);
    printf("System Uptime : %02ld:%02ld:%02ld\n", hours, mins, secs);
    printf("Load Averages : %.2f, %.2f, %.2f (1m, 5m, 15m)\n", sys.load_1m, sys.load_5m, sys.load_15m);
    printf("Active Tasks  : %d running/sleeping\n", sys.active_proc_count);
}

static void render_cpu(void)
{
    CpuObservation cpu = observe_cpu();

    printf("\nCPU\n");
    printf("----------------------------------------\n");
    printf("Processor     : %s\n", cpu.model_name);
    printf("CPU Cores     : %d\n", cpu.core_count);
    printf("Load Average  : %.2f (1m), %.2f (5m), %.2f (15m)\n", cpu.load_1m, cpu.load_5m, cpu.load_15m);
    printf("Ticks (User)  : %llu\n", cpu.user_ticks);
    printf("Ticks (System): %llu\n", cpu.sys_ticks);
    printf("Ticks (Idle)  : %llu\n", cpu.idle_ticks);
}

static void render_memory(void)
{
    MemoryObservation mem = observe_memory();

    printf("\nMEMORY\n");
    printf("----------------------------------------\n");
    printf("Total Memory  : %8ld MB\n", mem.mem_total_kb / 1024);
    printf("Used Memory   : %8ld MB (Total - Available)\n", mem.mem_used_kb / 1024);
    printf("Available     : %8ld MB\n", mem.mem_available_kb / 1024);
    printf("Free Memory   : %8ld MB\n", mem.mem_free_kb / 1024);
    printf("Buffers/Cache : %8ld MB\n", (mem.buffers_kb + mem.cached_kb) / 1024);
    if (mem.swap_total_kb > 0)
    {
        printf("Swap Total    : %8ld MB\n", mem.swap_total_kb / 1024);
        printf("Swap Free     : %8ld MB\n", mem.swap_free_kb / 1024);
    }
}

static void render_pid_info(pid_t pid)
{
    ProcessObservation proc = observe_process(pid);

    if (!proc.pid_exists)
    {
        printf("\nProcess ID %d could not be found or inspected.\n\n", pid);
        return;
    }

    printf("\nPROCESS INSPECTION (#%d)\n", pid);
    printf("----------------------------------------\n");
    printf("PID           : %d\n", proc.pid);
    printf("Name          : %s\n", proc.name);
    printf("State         : %s\n", proc.state);
    printf("Command Line  : %s\n", proc.cmdline);
    printf("Working Dir   : %s\n", proc.cwd);
    printf("Parent PID    : %d\n", proc.ppid);
    printf("User UID/GID  : %u / %u\n", proc.uid, proc.gid);
    printf("Threads       : %d\n", proc.threads);
    if (proc.vm_rss_kb > 0) printf("Memory (RSS)  : %ld KB\n", proc.vm_rss_kb);
    if (proc.vm_size_kb > 0) printf("Virtual Size  : %ld KB\n", proc.vm_size_kb);
    printf("\n");
}

static void render_overview(void)
{
    int cols = terminal_get_cols();

    printf("\nLiteShell System Monitor\n");
    printf("========================\n");

    render_system();
    render_cpu();
    render_memory();

    printf("\nPROCESS SNAPSHOT\n");
    if (cols >= 80)
    {
        printf("--------------------------------------------------------------------------------\n");
        printf("%-8s %-24s %-12s %-12s %-10s\n", "PID", "NAME", "STATE", "RSS (KB)", "ROLE");
        printf("--------------------------------------------------------------------------------\n");
    }
    else
    {
        printf("--------------------------------------------------\n");
        printf("%-8s %-20s %-12s\n", "PID", "NAME", "STATE");
        printf("--------------------------------------------------\n");
    }

    /* Show current shell process and top system tasks */
    ProcessObservation self_obs = observe_process(getpid());
    if (cols >= 80)
    {
        printf("%-8d %-24.24s %-12s %-12ld LiteShell\n", self_obs.pid, self_obs.name, self_obs.state, self_obs.vm_rss_kb);
    }
    else
    {
        printf("%-8d %-20.20s %-12s\n", self_obs.pid, self_obs.name, self_obs.state);
    }

    ProcessObservation init_obs = observe_process(1);
    if (init_obs.pid_exists)
    {
        if (cols >= 80)
        {
            printf("%-8d %-24.24s %-12s %-12ld Init\n", init_obs.pid, init_obs.name, init_obs.state, init_obs.vm_rss_kb);
        }
        else
        {
            printf("%-8d %-20.20s %-12s\n", init_obs.pid, init_obs.name, init_obs.state);
        }
    }

    SystemObservation sys = observe_system();
    printf("... (%d total processes active, use 'proc' or 'monitor process <PID>' for details)\n", sys.active_proc_count);
    printf("========================\n\n");
}

static void render_interactive(void)
{
    char choice[32];
    int done = 0;

    while (!done)
    {
        printf("\nLiteShell System Monitor\n");
        printf("========================\n");
        printf("[1] Overview\n");
        printf("[2] Processes\n");
        printf("[3] Memory\n");
        printf("[4] CPU\n");
        printf("[5] System\n");
        printf("[Q] Return to LiteShell\n\n");
        printf("Select option: ");
        fflush(stdout);

        if (fgets(choice, sizeof(choice), stdin) == NULL) break;
        choice[strcspn(choice, "\r\n")] = '\0';

        if (strcmp(choice, "1") == 0) render_overview();
        else if (strcmp(choice, "2") == 0)
        {
            printf("\nUse 'proc' or 'monitor process <PID>' for process inspection.\n");
        }
        else if (strcmp(choice, "3") == 0) render_memory();
        else if (strcmp(choice, "4") == 0) render_cpu();
        else if (strcmp(choice, "5") == 0) render_system();
        else if (strcmp(choice, "q") == 0 || strcmp(choice, "Q") == 0) done = 1;
        else printf("\nInvalid selection. Choose 1-5 or Q.\n");
    }
}

OperationResult ops_monitor(Command *cmd)
{
    OperationResult res;
    init_result(&res, "monitor", cmd);

    if (cmd->argc == 1)
    {
        render_overview();
        res.success = 1;
        res.exit_code = 0;
        return res;
    }

    const char *sub = cmd->args[1];
    if (strcmp(sub, "-i") == 0 || strcmp(sub, "menu") == 0 || strcmp(sub, "interactive") == 0)
    {
        render_interactive();
    }
    else if (strcmp(sub, "mem") == 0 || strcmp(sub, "memory") == 0)
    {
        render_memory();
    }
    else if (strcmp(sub, "cpu") == 0)
    {
        render_cpu();
    }
    else if (strcmp(sub, "sys") == 0 || strcmp(sub, "system") == 0)
    {
        render_system();
    }
    else if (strcmp(sub, "pid") == 0 || strcmp(sub, "process") == 0)
    {
        if (cmd->argc > 2)
        {
            pid_t target_pid = atoi(cmd->args[2]);
            render_pid_info(target_pid);
        }
        else
        {
            render_pid_info(getpid());
        }
    }
    else
    {
        render_overview();
    }

    res.success = 1;
    res.exit_code = 0;
    return res;
}
