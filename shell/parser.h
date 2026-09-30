#ifndef PARSER_H
#define PARSER_H

#define MAX_INPUT 1024
#define MAX_ARGS 64

typedef struct
{
    char *args[MAX_ARGS];
    int argc;
} Command;

int parse_command(char *input, Command *command);

#endif
