#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <errno.h>

#include "observation.h"

static void extract_parent_path(const char *path, char *parent_path, size_t max_len)
{
    if (path == NULL || parent_path == NULL || max_len == 0) return;

    strncpy(parent_path, path, max_len - 1);
    parent_path[max_len - 1] = '\0';

    char *last_slash = strrchr(parent_path, '/');
    if (last_slash != NULL)
    {
        if (last_slash == parent_path)
        {
            parent_path[1] = '\0';
        }
        else
        {
            *last_slash = '\0';
        }
    }
    else
    {
        strcpy(parent_path, ".");
    }
}

FilesystemObservation observe_filesystem(const char *path)
{
    FilesystemObservation obs;
    memset(&obs, 0, sizeof(FilesystemObservation));

    if (path == NULL || strlen(path) == 0) return obs;

    strncpy(obs.target_path, path, sizeof(obs.target_path) - 1);
    extract_parent_path(path, obs.parent_path, sizeof(obs.parent_path));

    struct stat st;
    if (stat(path, &st) == 0)
    {
        obs.path_exists = 1;
        obs.is_directory = S_ISDIR(st.st_mode) ? 1 : 0;
        obs.is_regular_file = S_ISREG(st.st_mode) ? 1 : 0;
        obs.file_size = st.st_size;
        obs.owner_uid = st.st_uid;
        obs.group_gid = st.st_gid;

        obs.read_perm = (access(path, R_OK) == 0) ? 1 : 0;
        obs.write_perm = (access(path, W_OK) == 0) ? 1 : 0;
        obs.exec_perm = (access(path, X_OK) == 0) ? 1 : 0;
    }
    else
    {
        obs.path_exists = 0;
    }

    struct stat pst;
    if (stat(obs.parent_path, &pst) == 0 && S_ISDIR(pst.st_mode))
    {
        obs.parent_exists = 1;
    }

    return obs;
}

ProcessObservation observe_process(pid_t pid)
{
    ProcessObservation obs;
    memset(&obs, 0, sizeof(ProcessObservation));

    if (pid <= 0) pid = getpid();
    obs.pid = pid;

    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/status", pid);

    FILE *f = fopen(proc_path, "r");
    if (f != NULL)
    {
        obs.pid_exists = 1;
        char line[256];
        while (fgets(line, sizeof(line), f))
        {
            if (strncmp(line, "Name:", 5) == 0) sscanf(line, "Name:\t%127s", obs.name);
            else if (strncmp(line, "State:", 6) == 0) sscanf(line, "State:\t%31s", obs.state);
            else if (strncmp(line, "PPid:", 5) == 0) sscanf(line, "PPid:\t%d", &obs.ppid);
            else if (strncmp(line, "Uid:", 4) == 0) sscanf(line, "Uid:\t%u", &obs.uid);
            else if (strncmp(line, "Gid:", 4) == 0) sscanf(line, "Gid:\t%u", &obs.gid);
            else if (strncmp(line, "Threads:", 8) == 0) sscanf(line, "Threads:\t%d", &obs.threads);
            else if (strncmp(line, "VmSize:", 7) == 0) sscanf(line, "VmSize:\t%ld", &obs.vm_size_kb);
            else if (strncmp(line, "VmRSS:", 6) == 0) sscanf(line, "VmRSS:\t%ld", &obs.vm_rss_kb);
        }
        fclose(f);
    }

    snprintf(proc_path, sizeof(proc_path), "/proc/%d/cwd", pid);
    ssize_t len = readlink(proc_path, obs.cwd, sizeof(obs.cwd) - 1);
    if (len > 0) obs.cwd[len] = '\0';
    else strcpy(obs.cwd, "Unknown");

    snprintf(proc_path, sizeof(proc_path), "/proc/%d/cmdline", pid);
    f = fopen(proc_path, "r");
    if (f != NULL)
    {
        size_t n = fread(obs.cmdline, 1, sizeof(obs.cmdline) - 1, f);
        fclose(f);
        if (n > 0)
        {
            obs.cmdline[n] = '\0';
            for (size_t i = 0; i < n - 1; i++)
            {
                if (obs.cmdline[i] == '\0') obs.cmdline[i] = ' ';
            }
        }
    }
    if (obs.cmdline[0] == '\0') strncpy(obs.cmdline, obs.name, sizeof(obs.cmdline) - 1);

    return obs;
}

MemoryObservation observe_memory(void)
{
    MemoryObservation obs;
    memset(&obs, 0, sizeof(MemoryObservation));

    FILE *f = fopen("/proc/meminfo", "r");
    if (f != NULL)
    {
        char line[256];
        while (fgets(line, sizeof(line), f))
        {
            if (strncmp(line, "MemTotal:", 9) == 0) sscanf(line, "MemTotal:\t%ld", &obs.mem_total_kb);
            else if (strncmp(line, "MemFree:", 8) == 0) sscanf(line, "MemFree:\t%ld", &obs.mem_free_kb);
            else if (strncmp(line, "MemAvailable:", 13) == 0) sscanf(line, "MemAvailable:\t%ld", &obs.mem_available_kb);
            else if (strncmp(line, "Buffers:", 8) == 0) sscanf(line, "Buffers:\t%ld", &obs.buffers_kb);
            else if (strncmp(line, "Cached:", 7) == 0) sscanf(line, "Cached:\t%ld", &obs.cached_kb);
            else if (strncmp(line, "SwapTotal:", 10) == 0) sscanf(line, "SwapTotal:\t%ld", &obs.swap_total_kb);
            else if (strncmp(line, "SwapFree:", 9) == 0) sscanf(line, "SwapFree:\t%ld", &obs.swap_free_kb);
        }
        fclose(f);
    }

    obs.mem_used_kb = (obs.mem_total_kb > obs.mem_available_kb) ? (obs.mem_total_kb - obs.mem_available_kb) : 0;
    return obs;
}

CpuObservation observe_cpu(void)
{
    CpuObservation obs;
    memset(&obs, 0, sizeof(CpuObservation));

    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    obs.core_count = (cores > 0) ? (int)cores : 1;
    strcpy(obs.model_name, "Generic x86_64");

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
                    strncpy(obs.model_name, colon + 2, sizeof(obs.model_name) - 1);
                    obs.model_name[sizeof(obs.model_name) - 1] = '\0';
                    obs.model_name[strcspn(obs.model_name, "\r\n")] = '\0';
                    break;
                }
            }
        }
        fclose(f_cpu);
    }

    FILE *f_load = fopen("/proc/loadavg", "r");
    if (f_load != NULL)
    {
        fscanf(f_load, "%lf %lf %lf", &obs.load_1m, &obs.load_5m, &obs.load_15m);
        fclose(f_load);
    }

    FILE *f_stat = fopen("/proc/stat", "r");
    if (f_stat != NULL)
    {
        char line[256];
        if (fgets(line, sizeof(line), f_stat))
        {
            if (strncmp(line, "cpu ", 4) == 0)
            {
                unsigned long long user, nice, sys, idle, iowait, irq, softirq;
                sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu",
                       &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
                obs.user_ticks = user + nice;
                obs.sys_ticks = sys + irq + softirq;
                obs.idle_ticks = idle + iowait;
            }
        }
        fclose(f_stat);
    }

    return obs;
}

static int is_numeric(const char *s)
{
    if (!s || !*s) return 0;
    while (*s)
    {
        if (!isdigit((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

SystemObservation observe_system(void)
{
    SystemObservation obs;
    memset(&obs, 0, sizeof(SystemObservation));

    gethostname(obs.hostname, sizeof(obs.hostname));

    struct utsname uts;
    if (uname(&uts) == 0)
    {
        strncpy(obs.sysname, uts.sysname, sizeof(obs.sysname) - 1);
        strncpy(obs.release, uts.release, sizeof(obs.release) - 1);
        strncpy(obs.machine, uts.machine, sizeof(obs.machine) - 1);
    }

    FILE *f_up = fopen("/proc/uptime", "r");
    if (f_up != NULL)
    {
        fscanf(f_up, "%lf", &obs.uptime_secs);
        fclose(f_up);
    }

    FILE *f_load = fopen("/proc/loadavg", "r");
    if (f_load != NULL)
    {
        fscanf(f_load, "%lf %lf %lf", &obs.load_1m, &obs.load_5m, &obs.load_15m);
        fclose(f_load);
    }

    DIR *dir = opendir("/proc");
    if (dir != NULL)
    {
        struct dirent *e;
        while ((e = readdir(dir)) != NULL)
        {
            if (is_numeric(e->d_name))
            {
                obs.active_proc_count++;
            }
        }
        closedir(dir);
    }

    return obs;
}

void print_filesystem_observation(const FilesystemObservation *obs)
{
    printf("Filesystem Observation:\n");
    printf("    Target path   : %s\n", obs->target_path[0] ? obs->target_path : "(none)");
    printf("    Path exists   : %s\n", obs->path_exists ? "YES" : "NO");
    if (obs->path_exists)
    {
        printf("    Type          : %s\n", obs->is_directory ? "Directory" : (obs->is_regular_file ? "Regular File" : "Other"));
        printf("    Size          : %ld bytes\n", (long)obs->file_size);
        printf("    Permissions   : Read=%s, Write=%s, Execute=%s\n",
               obs->read_perm ? "Yes" : "No",
               obs->write_perm ? "Yes" : "No",
               obs->exec_perm ? "Yes" : "No");
    }
    else
    {
        printf("    Parent dir    : %s (%s)\n", obs->parent_path, obs->parent_exists ? "EXISTS" : "MISSING");
    }
}

void print_system_observation(const ProcessObservation *obs)
{
    printf("Process & System Observation:\n");
    printf("    Process ID    : %d\n", obs->pid);
    printf("    Process State : %s\n", obs->state[0] ? obs->state : "Active/Terminated");
    printf("    Working Dir   : %s\n", obs->cwd);
    if (obs->vm_rss_kb > 0)
    {
        printf("    Memory (RSS)  : %ld KB\n", obs->vm_rss_kb);
    }
}
