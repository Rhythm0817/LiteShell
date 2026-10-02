#ifndef OBSERVATION_H
#define OBSERVATION_H

#include <sys/types.h>

typedef struct
{
    int path_exists;
    int is_directory;
    int is_regular_file;
    int parent_exists;
    int read_perm;
    int write_perm;
    int exec_perm;
    off_t file_size;
    uid_t owner_uid;
    gid_t group_gid;
    char target_path[256];
    char parent_path[256];
} FilesystemObservation;

typedef struct
{
    pid_t pid;
    int pid_exists;
    char name[128];
    char state[32];
    char exec_path[256];
    char cwd[256];
    char cmdline[256];
    pid_t ppid;
    uid_t uid;
    gid_t gid;
    int threads;
    long vm_size_kb;
    long vm_rss_kb;
} ProcessObservation;

typedef struct
{
    long mem_total_kb;
    long mem_free_kb;
    long mem_available_kb;
    long mem_used_kb;
    long buffers_kb;
    long cached_kb;
    long swap_total_kb;
    long swap_free_kb;
} MemoryObservation;

typedef struct
{
    int core_count;
    char model_name[128];
    double load_1m;
    double load_5m;
    double load_15m;
    unsigned long long user_ticks;
    unsigned long long sys_ticks;
    unsigned long long idle_ticks;
} CpuObservation;

typedef struct
{
    char hostname[128];
    char sysname[64];
    char release[64];
    char machine[64];
    double uptime_secs;
    int active_proc_count;
    double load_1m;
    double load_5m;
    double load_15m;
} SystemObservation;

/* Provider APIs */
FilesystemObservation observe_filesystem(const char *path);
ProcessObservation observe_process(pid_t pid);
MemoryObservation observe_memory(void);
CpuObservation observe_cpu(void);
SystemObservation observe_system(void);

void print_filesystem_observation(const FilesystemObservation *obs);
void print_system_observation(const ProcessObservation *obs);

#endif
