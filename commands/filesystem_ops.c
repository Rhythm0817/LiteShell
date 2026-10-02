#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "filesystem_ops.h"
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

/* ------------------------------------------------------------------ */
/* FILE OPERATIONS                                                    */
/* ------------------------------------------------------------------ */

OperationResult ops_makef(Command *cmd)
{
    OperationResult res;
    init_result(&res, "makef", cmd);

    if (cmd->argc < 2)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: makef <filename>");
        strcpy(res.error_message, "Missing filename argument for file creation.");
        return res;
    }

    const char *path = cmd->args[1];
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "open('%.200s', O_WRONLY|O_CREAT|O_EXCL) failed: %s", path, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot create file '%.150s': %s", path, strerror(errno));
        return res;
    }

    close(fd);
    printf("File '%.150s' created successfully.\n", path);
    res.success = 1;
    res.exit_code = 0;
    return res;
}

OperationResult ops_delf(Command *cmd)
{
    OperationResult res;
    init_result(&res, "delf", cmd);

    if (cmd->argc < 2)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: delf <filename>");
        strcpy(res.error_message, "Missing filename argument.");
        return res;
    }

    const char *path = cmd->args[1];
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EISDIR;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "unlink('%.200s') failed: Is a directory", path);
        snprintf(res.error_message, sizeof(res.error_message), "Cannot delete '%.150s': Is a directory. Use 'deldir' to remove directories.", path);
        return res;
    }

    if (unlink(path) != 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "unlink('%.200s') failed: %s", path, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot delete file '%.150s': %s", path, strerror(errno));
        return res;
    }

    printf("File '%.150s' deleted successfully.\n", path);
    res.success = 1;
    res.exit_code = 0;
    return res;
}

OperationResult ops_copyf(Command *cmd)
{
    OperationResult res;
    init_result(&res, "copyf", cmd);

    if (cmd->argc < 3)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: copyf <source> <destination>");
        strcpy(res.error_message, "Missing source or destination argument.");
        return res;
    }

    const char *src = cmd->args[1];
    const char *dst = cmd->args[2];

    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "open('%.200s', O_RDONLY) failed: %s", src, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot open source file '%.150s': %s", src, strerror(errno));
        return res;
    }

    struct stat st;
    if (fstat(src_fd, &st) == 0 && S_ISDIR(st.st_mode))
    {
        close(src_fd);
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EISDIR;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "source path '%.200s' is a directory", src);
        snprintf(res.error_message, sizeof(res.error_message), "'%.150s' is a directory, not a file. Use 'copydir' instead.", src);
        return res;
    }

    char final_dst[512];
    struct stat dst_st;
    if (stat(dst, &dst_st) == 0 && S_ISDIR(dst_st.st_mode))
    {
        const char *src_base = strrchr(src, '/');
        src_base = (src_base != NULL) ? src_base + 1 : src;
        snprintf(final_dst, sizeof(final_dst), "%s/%s", dst, src_base);
    }
    else
    {
        strncpy(final_dst, dst, sizeof(final_dst) - 1);
        final_dst[sizeof(final_dst) - 1] = '\0';
    }

    int dst_fd = open(final_dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0)
    {
        close(src_fd);
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "open('%.200s', O_WRONLY|O_CREAT) failed: %s", final_dst, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot create destination file '%.150s': %s", final_dst, strerror(errno));
        return res;
    }

    char buffer[4096];
    ssize_t bytes_read, bytes_written;
    int copy_failed = 0;

    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0)
    {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read)
        {
            copy_failed = 1;
            res.errno_val = errno;
            snprintf(res.stderr_output, sizeof(res.stderr_output), "write to '%.200s' failed: %s", final_dst, strerror(errno));
            snprintf(res.error_message, sizeof(res.error_message), "Write error copying to '%.150s'", final_dst);
            break;
        }
    }

    if (bytes_read < 0 && !copy_failed)
    {
        copy_failed = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "read from '%.200s' failed: %s", src, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Read error copying from '%.150s'", src);
    }

    close(src_fd);
    close(dst_fd);

    if (copy_failed)
    {
        res.success = 0;
        res.exit_code = 1;
    }
    else
    {
        res.success = 1;
        res.exit_code = 0;
        printf("File copied successfully to '%.150s'.\n", final_dst);
    }

    return res;
}

OperationResult ops_movef(Command *cmd)
{
    OperationResult res;
    init_result(&res, "movef", cmd);

    if (cmd->argc < 3)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: movef <source> <destination>");
        strcpy(res.error_message, "Missing source or destination argument.");
        return res;
    }

    const char *src = cmd->args[1];
    const char *dst = cmd->args[2];

    if (rename(src, dst) == 0)
    {
        res.success = 1;
        res.exit_code = 0;
        printf("File moved successfully to '%.150s'.\n", dst);
    }
    else
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "rename('%.200s', '%.200s') failed: %s", src, dst, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot move '%.100s' to '%.100s': %s", src, dst, strerror(errno));
    }

    return res;
}

OperationResult ops_typef(Command *cmd)
{
    OperationResult res;
    init_result(&res, "typef", cmd);

    if (cmd->argc < 2)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: typef <filename>");
        strcpy(res.error_message, "Missing filename argument.");
        return res;
    }

    const char *path = cmd->args[1];
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "open('%.200s', O_RDONLY) failed: %s", path, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot open file '%.150s': %s", path, strerror(errno));
        return res;
    }

    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        write(STDOUT_FILENO, buffer, bytes_read);
    }
    close(fd);

    res.success = 1;
    res.exit_code = 0;
    return res;
}

/* ------------------------------------------------------------------ */
/* DIRECTORY OPERATIONS                                               */
/* ------------------------------------------------------------------ */

OperationResult ops_makedir(Command *cmd)
{
    OperationResult res;
    init_result(&res, "makedir", cmd);

    if (cmd->argc < 2)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: makedir <directory>");
        strcpy(res.error_message, "Missing directory argument.");
        return res;
    }

    const char *path = cmd->args[1];
    if (mkdir(path, 0755) != 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "mkdir('%.200s', 0755) failed: %s", path, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot create directory '%.150s': %s", path, strerror(errno));
        return res;
    }

    printf("Directory '%.150s' created successfully.\n", path);
    res.success = 1;
    res.exit_code = 0;
    return res;
}

OperationResult ops_deldir(Command *cmd)
{
    OperationResult res;
    init_result(&res, "deldir", cmd);

    if (cmd->argc < 2)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: deldir <directory>");
        strcpy(res.error_message, "Missing directory argument.");
        return res;
    }

    const char *path = cmd->args[1];
    if (rmdir(path) != 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "rmdir('%.200s') failed: %s", path, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot remove directory '%.150s': %s", path, strerror(errno));
        return res;
    }

    printf("Directory '%.150s' removed successfully.\n", path);
    res.success = 1;
    res.exit_code = 0;
    return res;
}

OperationResult ops_copydir(Command *cmd)
{
    OperationResult res;
    init_result(&res, "copydir", cmd);

    if (cmd->argc < 3)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: copydir <source> <destination>");
        strcpy(res.error_message, "Missing source or destination directory argument.");
        return res;
    }

    const char *src = cmd->args[1];
    const char *dst = cmd->args[2];

    DIR *dir = opendir(src);
    if (dir == NULL)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "opendir('%.200s') failed: %s", src, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot open source directory '%.150s': %s", src, strerror(errno));
        return res;
    }

    if (mkdir(dst, 0755) != 0 && errno != EEXIST)
    {
        closedir(dir);
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "mkdir('%.200s') failed: %s", dst, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot create destination directory '%.150s': %s", dst, strerror(errno));
        return res;
    }

    struct dirent *entry;
    int files_copied = 0;
    int copy_errors = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char src_path[512];
        char dst_path[512];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

        struct stat st;
        if (stat(src_path, &st) == 0)
        {
            if (S_ISREG(st.st_mode))
            {
                int sfd = open(src_path, O_RDONLY);
                int dfd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (sfd >= 0 && dfd >= 0)
                {
                    char buf[4096];
                    ssize_t nr;
                    while ((nr = read(sfd, buf, sizeof(buf))) > 0)
                    {
                        write(dfd, buf, nr);
                    }
                    close(sfd);
                    close(dfd);
                    files_copied++;
                }
                else
                {
                    if (sfd >= 0) close(sfd);
                    if (dfd >= 0) close(dfd);
                    copy_errors++;
                }
            }
        }
    }
    closedir(dir);

    if (copy_errors > 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EIO;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "copydir failed with %d file copy errors", copy_errors);
        snprintf(res.error_message, sizeof(res.error_message), "Partial directory copy failure.");
        return res;
    }

    printf("Directory copied successfully to '%.150s' (%d file(s) copied).\n", dst, files_copied);
    res.success = 1;
    res.exit_code = 0;
    return res;
}

OperationResult ops_movedir(Command *cmd)
{
    OperationResult res;
    init_result(&res, "movedir", cmd);

    if (cmd->argc < 3)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: movedir <source> <destination>");
        strcpy(res.error_message, "Missing source or destination directory argument.");
        return res;
    }

    const char *src = cmd->args[1];
    const char *dst = cmd->args[2];

    if (rename(src, dst) == 0)
    {
        res.success = 1;
        res.exit_code = 0;
        printf("Directory moved successfully to '%.150s'.\n", dst);
    }
    else
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "rename('%.200s', '%.200s') failed: %s", src, dst, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot move directory '%.100s' to '%.100s': %s", src, dst, strerror(errno));
    }

    return res;
}

OperationResult ops_dir(Command *cmd)
{
    OperationResult res;
    init_result(&res, "dir", cmd);

    const char *target = (cmd->argc > 1) ? cmd->args[1] : ".";
    DIR *dir = opendir(target);

    if (dir == NULL)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "opendir('%.200s') failed: %s", target, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot open directory '%.150s': %s", target, strerror(errno));
        return res;
    }

    int cols = terminal_get_cols();
    printf("\n Directory of %s\n\n", target);

    if (cols >= 80)
    {
        printf("%-24s %-8s %-12s %-18s\n", "NAME", "TYPE", "SIZE", "MODIFIED");
        printf("--------------------------------------------------------------------------------\n");
    }
    else
    {
        printf("%-24s %-8s %-10s\n", "NAME", "TYPE", "SIZE");
        printf("--------------------------------------------------\n");
    }

    struct dirent *entry;
    int file_count = 0;
    int dir_count = 0;
    off_t total_bytes = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", target, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0)
        {
            struct tm *tm_info = localtime(&st.st_mtime);
            char date_str[32];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", tm_info);

            if (S_ISDIR(st.st_mode))
            {
                dir_count++;
                if (cols >= 80)
                {
                    printf("%-24.24s %-8s %-12s %-18s\n", entry->d_name, "<DIR>", "--", date_str);
                }
                else
                {
                    printf("%-24.24s %-8s %-10s\n", entry->d_name, "<DIR>", "--");
                }
            }
            else
            {
                file_count++;
                total_bytes += st.st_size;
                char size_str[32];
                snprintf(size_str, sizeof(size_str), "%ld B", (long)st.st_size);

                if (cols >= 80)
                {
                    printf("%-24.24s %-8s %-12s %-18s\n", entry->d_name, "FILE", size_str, date_str);
                }
                else
                {
                    printf("%-24.24s %-8s %-10s\n", entry->d_name, "FILE", size_str);
                }
            }
        }
        else
        {
            printf("%-24.24s %-8s %-10s\n", entry->d_name, "???", "--");
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
    printf("Total: %d File(s) (%ld bytes), %d Dir(s)\n\n", file_count, (long)total_bytes, dir_count);

    res.success = 1;
    res.exit_code = 0;
    return res;
}

/* ------------------------------------------------------------------ */
/* NAVIGATION OPERATIONS                                              */
/* ------------------------------------------------------------------ */

OperationResult ops_cdir(Command *cmd)
{
    OperationResult res;
    init_result(&res, "cdir", cmd);

    const char *target = (cmd->argc > 1) ? cmd->args[1] : getenv("HOME");
    if (target == NULL)
    {
        target = "/";
    }

    if (chdir(target) != 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "chdir('%.200s') failed: %s", target, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot change directory to '%.150s': %s", target, strerror(errno));
        return res;
    }

    res.success = 1;
    res.exit_code = 0;
    return res;
}

OperationResult ops_curdir(Command *cmd)
{
    OperationResult res;
    init_result(&res, "curdir", cmd);

    char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "getcwd() failed: %s", strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Failed to retrieve current directory: %s", strerror(errno));
        return res;
    }

    printf("%s\n", cwd);
    res.success = 1;
    res.exit_code = 0;
    return res;
}

/* ------------------------------------------------------------------ */
/* ALIASES / BACKWARD COMPATIBILITY WRAPPERS                          */
/* ------------------------------------------------------------------ */

OperationResult ops_cd(Command *cmd)
{
    return ops_cdir(cmd);
}

OperationResult ops_pwd(Command *cmd)
{
    return ops_curdir(cmd);
}

OperationResult ops_mkdir(Command *cmd)
{
    return ops_makedir(cmd);
}

OperationResult ops_rmdir(Command *cmd)
{
    return ops_deldir(cmd);
}

OperationResult ops_copy(Command *cmd)
{
    return ops_copyf(cmd);
}

OperationResult ops_move(Command *cmd)
{
    return ops_movef(cmd);
}

OperationResult ops_del(Command *cmd)
{
    return ops_delf(cmd);
}

OperationResult ops_type(Command *cmd)
{
    return ops_typef(cmd);
}
