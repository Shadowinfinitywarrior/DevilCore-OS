// shell/src/parser.c
// Command parser

#include <devil/types.h>
#include <string.h>

#define MAX_ARGS 64
#define MAX_TOKENS 128

typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_REDIR_APPEND,
    TOKEN_BG,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_SEMICOLON,
} token_type_t;

typedef struct {
    token_type_t type;
    char value[256];
} token_t;

static token_t tokens[MAX_TOKENS];
static int token_count;

int parse_command(const char *cmd, char **argv) {
    int argc = 0;
    char buf[1024];
    int pos = 0;
    
    for (int i = 0; cmd[i] && i < 1024; i++) {
        if (cmd[i] == ' ' || cmd[i] == '\t') {
            if (pos > 0) {
                buf[pos] = '\0';
                argv[argc++] = strdup(buf);
                pos = 0;
            }
        } else if (cmd[i] == '"') {
            i++;
            while (cmd[i] && cmd[i] != '"') {
                buf[pos++] = cmd[i++];
            }
        } else {
            buf[pos++] = cmd[i];
        }
    }
    
    if (pos > 0) {
        buf[pos] = '\0';
        argv[argc++] = strdup(buf);
    }
    
    argv[argc] = NULL;
    return argc;
}