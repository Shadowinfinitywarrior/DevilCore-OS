// shell/src/executor.c
// Command executor

#include <devil/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

static int execute_command(char **args, int stdin, int stdout) {
    if (!args[0]) return 0;
    
    // Internal commands
    if (!strcmp(args[0], "cd")) {
        if (args[1]) chdir(args[1]);
        return 0;
    }
    
    if (!strcmp(args[0], "exit") || !strcmp(args[0], "quit")) {
        exit(args[1] ? atoi(args[1]) : 0);
        return 0;
    }
    
    // External commands
    pid_t pid = fork();
    if (pid == 0) {
        if (stdin != STDIN_FILENO) {
            dup2(stdin, STDIN_FILENO);
            close(stdin);
        }
        if (stdout != STDOUT_FILENO) {
            dup2(stdout, STDOUT_FILENO);
            close(stdout);
        }
        execvp(args[0], args);
        exit(127);
    }
    
    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}