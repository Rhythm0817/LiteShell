#ifndef TERMINAL_H
#define TERMINAL_H

#include <signal.h>

typedef struct {
    int rows;
    int cols;
} TerminalSize;

/* Global atomic flag set when SIGWINCH signal is received */
extern volatile sig_atomic_t g_terminal_resized;

void terminal_init(void);
TerminalSize terminal_get_size(void);
int terminal_get_cols(void);
int terminal_get_rows(void);
int terminal_check_resize(void);
void terminal_clear_screen(void);
void terminal_move_cursor(int row, int col);

#endif
