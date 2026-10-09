#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "parser.h"

void expand_variables(const char *input, char *output, size_t max_len, int last_exit_code)
{
    if (input == NULL || output == NULL || max_len == 0) return;

    size_t out_idx = 0;
    size_t in_len = strlen(input);

    for (size_t i = 0; i < in_len && out_idx + 1 < max_len; )
    {
        if (input[i] == '$' && i + 1 < in_len)
        {
            if (input[i + 1] == '?')
            {
                char num_buf[32];
                snprintf(num_buf, sizeof(num_buf), "%d", last_exit_code);
                size_t num_len = strlen(num_buf);
                for (size_t j = 0; j < num_len && out_idx + 1 < max_len; j++)
                {
                    output[out_idx++] = num_buf[j];
                }
                i += 2;
                continue;
            }
            else if (input[i + 1] == '$')
            {
                char num_buf[32];
                snprintf(num_buf, sizeof(num_buf), "%d", (int)getpid());
                size_t num_len = strlen(num_buf);
                for (size_t j = 0; j < num_len && out_idx + 1 < max_len; j++)
                {
                    output[out_idx++] = num_buf[j];
                }
                i += 2;
                continue;
            }
            else if (input[i + 1] == '{')
            {
                size_t end = i + 2;
                while (end < in_len && input[end] != '}')
                {
                    end++;
                }
                if (end < in_len && input[end] == '}')
                {
                    char var_name[128];
                    size_t vlen = end - (i + 2);
                    if (vlen >= sizeof(var_name)) vlen = sizeof(var_name) - 1;
                    strncpy(var_name, &input[i + 2], vlen);
                    var_name[vlen] = '\0';

                    const char *val = getenv(var_name);
                    if (val != NULL)
                    {
                        while (*val != '\0' && out_idx + 1 < max_len)
                        {
                            output[out_idx++] = *val++;
                        }
                    }
                    i = end + 1;
                    continue;
                }
            }
            else if (isalpha((unsigned char)input[i + 1]) || input[i + 1] == '_')
            {
                size_t start = i + 1;
                size_t end = start;
                while (end < in_len && (isalnum((unsigned char)input[end]) || input[end] == '_'))
                {
                    end++;
                }
                char var_name[128];
                size_t vlen = end - start;
                if (vlen >= sizeof(var_name)) vlen = sizeof(var_name) - 1;
                strncpy(var_name, &input[start], vlen);
                var_name[vlen] = '\0';

                const char *val = getenv(var_name);
                if (val != NULL)
                {
                    while (*val != '\0' && out_idx + 1 < max_len)
                    {
                        output[out_idx++] = *val++;
                    }
                }
                i = end;
                continue;
            }
        }

        output[out_idx++] = input[i++];
    }

    output[out_idx] = '\0';
}

int parse_command(char *input, Command *command)
{
    if (input == NULL || command == NULL) return 0;

    memset(command, 0, sizeof(Command));

    size_t read_idx = 0;
    size_t write_idx = 0;
    int redir_mode = 0; /* 0: normal arg, 1: '<' input, 2: '>' output, 3: '>>' append */

    while (input[read_idx] != '\0')
    {
        /* Skip whitespace */
        while (input[read_idx] == ' ' || input[read_idx] == '\t' || input[read_idx] == '\r' || input[read_idx] == '\n')
        {
            read_idx++;
        }

        if (input[read_idx] == '\0') break;

        /* Check redirection tokens */
        if (input[read_idx] == '<')
        {
            read_idx++;
            redir_mode = 1;
            continue;
        }

        if (input[read_idx] == '>')
        {
            read_idx++;
            if (input[read_idx] == '>')
            {
                read_idx++;
                redir_mode = 3;
            }
            else
            {
                redir_mode = 2;
            }
            continue;
        }

        /* Token start */
        char *tok = &input[write_idx];
        int in_single = 0;
        int in_double = 0;

        while (input[read_idx] != '\0')
        {
            char c = input[read_idx++];

            if (c == '\\' && !in_single)
            {
                if (input[read_idx] != '\0')
                {
                    input[write_idx++] = input[read_idx++];
                }
            }
            else if (c == '\'' && !in_double)
            {
                in_single = !in_single;
            }
            else if (c == '"' && !in_single)
            {
                in_double = !in_double;
            }
            else if ((c == ' ' || c == '\t' || c == '\r' || c == '\n') && !in_single && !in_double)
            {
                break;
            }
            else if ((c == '<' || c == '>') && !in_single && !in_double)
            {
                /* Hit redirection without preceding whitespace (e.g. dir>out.txt) */
                read_idx--;
                break;
            }
            else
            {
                input[write_idx++] = c;
            }
        }

        input[write_idx++] = '\0';

        if (redir_mode == 1)
        {
            command->input_file = tok;
            redir_mode = 0;
        }
        else if (redir_mode == 2)
        {
            command->output_file = tok;
            command->append_output = 0;
            redir_mode = 0;
        }
        else if (redir_mode == 3)
        {
            command->output_file = tok;
            command->append_output = 1;
            redir_mode = 0;
        }
        else
        {
            if (command->argc < MAX_ARGS - 1)
            {
                command->args[command->argc++] = tok;
            }
        }
    }

    command->args[command->argc] = NULL;
    return command->argc;
}
