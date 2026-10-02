#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "native_ops.h"

int is_native_command(const char *cmd_name)
{
    if (cmd_name == NULL) return 0;
    if (strcmp(cmd_name, "copy") == 0 ||
        strcmp(cmd_name, "move") == 0 ||
        strcmp(cmd_name, "remove") == 0 ||
        strcmp(cmd_name, "mkdir") == 0 ||
        strcmp(cmd_name, "cat") == 0)
    {
        return 1;
    }
    return 0;
}

static OperationResult handle_copy(Command *command)
{
    OperationResult res;
    memset(&res, 0, sizeof(OperationResult));
    res.is_native = 1;
    res.pid = getpid();
    strcpy(res.command, "copy");
    
    snprintf(res.raw_args, sizeof(res.raw_args), "copy %s %s",
             command->argc > 1 ? command->args[1] : "",
             command->argc > 2 ? command->args[2] : "");

    if (command->argc < 3)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: copy <source_file> <destination_path>");
        strcpy(res.error_message, "Missing arguments for native copy operation.");
        return res;
    }

    const char *src = command->args[1];
    const char *dst = command->args[2];

    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "open('%s', O_RDONLY) failed: %s", src, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot open source file '%s': %s", src, strerror(errno));
        return res;
    }

    struct stat st;
    if (fstat(src_fd, &st) == 0 && S_ISDIR(st.st_mode))
    {
        close(src_fd);
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EISDIR;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "source path '%s' is a directory", src);
        snprintf(res.error_message, sizeof(res.error_message), "'%s' is a directory, not a regular file.", src);
        return res;
    }

    /* Check if destination is a directory */
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
            snprintf(res.error_message, sizeof(res.error_message), "Write error copying to '%.200s'", final_dst);
            break;
        }
    }


    if (bytes_read < 0 && !copy_failed)
    {
        copy_failed = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "read from '%s' failed: %s", src, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Read error copying from '%s'", src);
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
        printf("Copied '%s' -> '%s'\n", src, final_dst);
    }

    return res;
}

static OperationResult handle_move(Command *command)
{
    OperationResult res;
    memset(&res, 0, sizeof(OperationResult));
    res.is_native = 1;
    res.pid = getpid();
    strcpy(res.command, "move");

    snprintf(res.raw_args, sizeof(res.raw_args), "move %s %s",
             command->argc > 1 ? command->args[1] : "",
             command->argc > 2 ? command->args[2] : "");

    if (command->argc < 3)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: move <source_path> <destination_path>");
        strcpy(res.error_message, "Missing arguments for native move operation.");
        return res;
    }

    const char *src = command->args[1];
    const char *dst = command->args[2];

    if (rename(src, dst) == 0)
    {
        res.success = 1;
        res.exit_code = 0;
        printf("Moved '%s' -> '%s'\n", src, dst);
    }
    else
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "rename('%s', '%s') failed: %s", src, dst, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot move '%s' to '%s': %s", src, dst, strerror(errno));
    }

    return res;
}

static OperationResult handle_remove(Command *command)
{
    OperationResult res;
    memset(&res, 0, sizeof(OperationResult));
    res.is_native = 1;
    res.pid = getpid();
    strcpy(res.command, "remove");

    snprintf(res.raw_args, sizeof(res.raw_args), "remove %s", command->argc > 1 ? command->args[1] : "");

    if (command->argc < 2)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: remove <target_path>");
        strcpy(res.error_message, "Missing target path for native remove operation.");
        return res;
    }

    const char *path = command->args[1];
    struct stat st;
    int is_dir = 0;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
    {
        is_dir = 1;
    }

    int ret = is_dir ? rmdir(path) : unlink(path);
    if (ret == 0)
    {
        res.success = 1;
        res.exit_code = 0;
        printf("Removed '%s'\n", path);
    }
    else
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "%s('%s') failed: %s", is_dir ? "rmdir" : "unlink", path, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot remove '%s': %s", path, strerror(errno));
    }

    return res;
}

static OperationResult handle_mkdir(Command *command)
{
    OperationResult res;
    memset(&res, 0, sizeof(OperationResult));
    res.is_native = 1;
    res.pid = getpid();
    strcpy(res.command, "mkdir");

    snprintf(res.raw_args, sizeof(res.raw_args), "mkdir %s", command->argc > 1 ? command->args[1] : "");

    if (command->argc < 2)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: mkdir <directory_path>");
        strcpy(res.error_message, "Missing directory path for native mkdir operation.");
        return res;
    }

    const char *path = command->args[1];
    if (mkdir(path, 0755) == 0)
    {
        res.success = 1;
        res.exit_code = 0;
        printf("Created directory '%s'\n", path);
    }
    else
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "mkdir('%s', 0755) failed: %s", path, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot create directory '%s': %s", path, strerror(errno));
    }

    return res;
}

static OperationResult handle_cat(Command *command)
{
    OperationResult res;
    memset(&res, 0, sizeof(OperationResult));
    res.is_native = 1;
    res.pid = getpid();
    strcpy(res.command, "cat");

    snprintf(res.raw_args, sizeof(res.raw_args), "cat %s", command->argc > 1 ? command->args[1] : "");

    if (command->argc < 2)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = EINVAL;
        strcpy(res.stderr_output, "Usage: cat <file_path>");
        strcpy(res.error_message, "Missing file path for native cat operation.");
        return res;
    }

    const char *path = command->args[1];
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        res.success = 0;
        res.exit_code = 1;
        res.errno_val = errno;
        snprintf(res.stderr_output, sizeof(res.stderr_output), "open('%s', O_RDONLY) failed: %s", path, strerror(errno));
        snprintf(res.error_message, sizeof(res.error_message), "Cannot open file '%s': %s", path, strerror(errno));
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

OperationResult execute_native_command(Command *command)
{
    if (strcmp(command->args[0], "copy") == 0) return handle_copy(command);
    if (strcmp(command->args[0], "move") == 0) return handle_move(command);
    if (strcmp(command->args[0], "remove") == 0) return handle_remove(command);
    if (strcmp(command->args[0], "mkdir") == 0) return handle_mkdir(command);
    if (strcmp(command->args[0], "cat") == 0) return handle_cat(command);

    OperationResult empty;
    memset(&empty, 0, sizeof(OperationResult));
    return empty;
}
