#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "terminal.h"

volatile sig_atomic_t g_terminal_resized = 0;
static TerminalSize g_cached_size = {25, 80};

static void sigwinch_handler(int sig)
{
    (void)sig;
    g_terminal_resized = 1;
}

TerminalSize terminal_get_size(void)
{
    TerminalSize size = {25, 80};
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0)
    {
        size.rows = ws.ws_row;
        size.cols = ws.ws_col;
    }
    else
    {
        /* Fallback to environment variables if available */
        char *env_cols = getenv("COLUMNS");
        char *env_rows = getenv("LINES");
        if (env_cols && atoi(env_cols) > 0)
        {
            size.cols = atoi(env_cols);
        }
        if (env_rows && atoi(env_rows) > 0)
        {
            size.rows = atoi(env_rows);
        }
    }

    g_cached_size = size;
    return size;
}

void terminal_init(void)
{
    /* Query initial terminal size */
    terminal_get_size();

    /* Register SIGWINCH signal handler for terminal resize */
    struct sigaction sa;
    sa.sa_handler = sigwinch_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, NULL);
}

int terminal_get_cols(void)
{
    if (g_terminal_resized)
    {
        terminal_get_size();
        g_terminal_resized = 0;
    }
    return g_cached_size.cols;
}

int terminal_get_rows(void)
{
    if (g_terminal_resized)
    {
        terminal_get_size();
        g_terminal_resized = 0;
    }
    return g_cached_size.rows;
}

int terminal_check_resize(void)
{
    if (g_terminal_resized)
    {
        g_terminal_resized = 0;
        terminal_get_size();
        return 1;
    }
    return 0;
}

void terminal_clear_screen(void)
{
    printf("\033[H\033[2J");
    fflush(stdout);
}

void terminal_move_cursor(int row, int col)
{
    printf("\033[%d;%dH", row, col);
    fflush(stdout);
}
