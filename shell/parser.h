#ifndef PARSER_H
#define PARSER_H

#ifndef MAX_INPUT_LINE
#define MAX_INPUT_LINE 1024
#endif

#define MAX_ARGS 64

typedef struct
{
    char *args[MAX_ARGS];
    int argc;
} Command;

int parse_command(char *input, Command *command);

#endif

