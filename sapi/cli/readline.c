#include <stdio.h>
#include <stdlib.h>

#ifdef SL_HAS_READLINE

#include <readline/readline.h>

void
cli_setup_readline()
{
}

char*
cli_readline(char* prompt)
{
    // todo - figure out how readline history works and implement that
    char* line = readline(prompt);
    if(line != NULL && line[0] != 0) {
        add_history(line);
    }
    return line;
}

#else

void
cli_setup_readline()
{
}

char*
cli_readline(char* prompt)
{
    printf("%s", prompt);
    fflush(stdout);

    size_t buff_cap = 32;
    size_t buff_size = 0;
    char* line = malloc(buff_cap);
    while(1) {
        int c = getc(stdin);
        if(c == EOF) {
            if(buff_size == 0) {
                free(line);
                return NULL;
            } else {
                line[buff_size] = 0;
                return line;
            }
        }
        if(c == '\n') {
            line[buff_size] = 0;
            return line;
        }
        if(buff_size + 2 >= buff_cap) {
            buff_cap *= 2;
            line = realloc(line, buff_cap);
        }
        line[buff_size++] = c;
    }
}

#endif
