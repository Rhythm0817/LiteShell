#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "screen.h"
#include "terminal.h"
#include "../observation/observation.h"

int screen_is_wide_mode(void)
{
    return (terminal_get_cols() >= 80);
}

void screen_draw_separator(char ch)
{
    int cols = terminal_get_cols();
    if (cols < 20) cols = 20;

    for (int i = 0; i < cols; i++)
    {
        putchar(ch);
    }
    putchar('\n');
}

void screen_render_header(void)
{
    int cols = terminal_get_cols();
    SystemObservation sys = observe_system();
    MemoryObservation mem = observe_memory();

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[32];
    char time_str[32];
    strftime(date_str, sizeof(date_str), "%d-%m-%Y", tm_info);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

    long hours = (long)sys.uptime_secs / 3600;
    long mins = ((long)sys.uptime_secs % 3600) / 60;
    long secs = (long)sys.uptime_secs % 60;

    char cwd[256] = "";
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        strcpy(cwd, "unknown");
    }

    if (cols >= 80)
    {
        /* Expanded Full-Terminal Layout */
        screen_draw_separator('+');

        char line1[512];
        snprintf(line1, sizeof(line1), "| LiteShell OS v0.4                             Date: %s  Time: %s", date_str, time_str);
        printf("%-*s|\n", cols - 1, line1);

        char line2[512];
        snprintf(line2, sizeof(line2), "| Host: %-12s  Uptime: %02ld:%02ld:%02ld  Mem: %ld/%ld MB  Tasks: %d",
                 sys.hostname[0] ? sys.hostname : "localhost",
                 hours, mins, secs,
                 mem.mem_used_kb / 1024, mem.mem_total_kb / 1024,
                 sys.active_proc_count);
        printf("%-*s|\n", cols - 1, line2);

        char line3[512];
        snprintf(line3, sizeof(line3), "| CWD: %.240s", cwd);
        printf("%-*s|\n", cols - 1, line3);

        screen_draw_separator('+');
    }
    else
    {
        /* Compact Small-Terminal Layout */
        screen_draw_separator('-');
        printf("LiteShell OS v0.4 | %s %s\n", date_str, time_str);
        printf("CWD: %.60s\n", cwd);
        screen_draw_separator('-');
    }
}

void screen_render_footer(void)
{
    int cols = terminal_get_cols();

    if (cols >= 80)
    {
        screen_draw_separator('-');
        printf(" HELP: Command reference   MONITOR: System status   DIAGNOSE: Explain failure   EXIT: Quit\n");
        screen_draw_separator('-');
    }
    else
    {
        screen_draw_separator('-');
        printf("Type HELP for commands | EXIT to quit\n");
        screen_draw_separator('-');
    }
}

void screen_render_prompt(void)
{
    printf("LITESHELL> ");
    fflush(stdout);
}
