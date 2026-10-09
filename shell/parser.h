#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#ifndef MAX_INPUT_LINE
#define MAX_INPUT_LINE 1024
#endif

#define MAX_ARGS 64

typedef struct
{
    char *args[MAX_ARGS];
    int argc;
    char *input_file;      /* Target path for '<' input redirection */
    char *output_file;     /* Target path for '>' or '>>' output redirection */
    int append_output;     /* 1 for '>>' (append), 0 for '>' (truncate) */
} Command;

/* Parse command string into Command struct, supporting single/double quotes and I/O redirection */
int parse_command(char *input, Command *command);

/* Expand environment variables ($VAR, ${VAR}, $?) in input buffer */
void expand_variables(const char *input, char *output, size_t max_len, int last_exit_code);

#endif

