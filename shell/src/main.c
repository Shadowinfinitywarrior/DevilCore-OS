/*
 * DevilShell v2.0 - Full-Featured Terminal
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

static char cwd[4096] = "/";
static char home[4096] = "/root";
static char history[1000][256];
static int history_count = 0;

char *expand_tilde(char *path) {
    static char buf[4096];
    if (path[0] == '~') {
        snprintf(buf, sizeof(buf), "%s/%s", home, path + 1);
        return buf;
    }
    strncpy(buf, path, sizeof(buf) - 1);
    return buf;
}

int cmd_ls(int ac, char **av) {
    char *path = ".";
    for (int i = 1; i < ac; i++) {
        if (av[i][0] == '-') continue;
        path = av[i];
    }
    path = expand_tilde(path);
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "ls: cannot access '%s'\n", path);
        return 1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        printf("%s  ", entry->d_name);
    }
    printf("\n");
    closedir(dir);
    return 0;
}

int cmd_cat(int ac, char **av) {
    if (ac < 2) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), stdin) != NULL) {
            fputs(buf, stdout);
        }
        return 0;
    }
    for (int i = 1; i < ac; i++) {
        char *path = expand_tilde(av[i]);
        FILE *f = fopen(path, "r");
        if (!f) {
            fprintf(stderr, "cat: %s: No such file\n", path);
            continue;
        }
        char buf[4096];
        while (fgets(buf, sizeof(buf), f) != NULL) {
            fputs(buf, stdout);
        }
        fclose(f);
    }
    return 0;
}

int cmd_cd(int ac, char **av) {
    char *path = (ac < 2) ? home : expand_tilde(av[1]);
    int ret = chdir(path);
    if (ret != 0) {
        fprintf(stderr, "cd: %s: %s\n", av[1] ? av[1] : home, strerror(errno));
        return 1;
    }
    getcwd(cwd, sizeof(cwd));
    return 0;
}

int cmd_pwd(int ac, char **av) {
    (void)ac; (void)av;
    printf("%s\n", cwd);
    return 0;
}

int cmd_echo(int ac, char **av) {
    for (int i = 1; i < ac; i++) {
        if (i > 1) printf(" ");
        printf("%s", av[i]);
    }
    printf("\n");
    return 0;
}

int cmd_mkdir(int ac, char **av) {
    if (ac < 2) {
        fprintf(stderr, "mkdir: missing operand\n");
        return 1;
    }
    for (int i = 1; i < ac; i++) {
        char *path = expand_tilde(av[i]);
        if (mkdir(path, 0755) != 0) {
            fprintf(stderr, "mkdir: cannot create directory '%s'\n", path);
            return 1;
        }
    }
    return 0;
}

int cmd_rm(int ac, char **av) {
    if (ac < 2) {
        fprintf(stderr, "rm: missing operand\n");
        return 1;
    }
    for (int i = 1; i < ac; i++) {
        if (unlink(av[i]) != 0) {
            fprintf(stderr, "rm: cannot remove '%s'\n", av[i]);
        }
    }
    return 0;
}

int cmd_cp(int ac, char **av) {
    if (ac < 3) {
        fprintf(stderr, "cp: missing operand\n");
        return 1;
    }
    FILE *src = fopen(av[1], "rb");
    if (!src) {
        fprintf(stderr, "cp: cannot stat '%s'\n", av[1]);
        return 1;
    }
    FILE *dst = fopen(av[2], "wb");
    if (!dst) {
        fprintf(stderr, "cp: cannot create '%s'\n", av[2]);
        fclose(src);
        return 1;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) != 0) {
        if (n != fwrite(buf, 1, n, dst)) break;
    }
    fclose(src);
    fclose(dst);
    return 0;
}

int cmd_mv(int ac, char **av) {
    if (ac < 3) {
        fprintf(stderr, "mv: missing operand\n");
        return 1;
    }
    if (rename(av[1], av[2]) != 0) {
        fprintf(stderr, "mv: cannot move '%s'\n", av[1]);
        return 1;
    }
    return 0;
}

int cmd_ps(int ac, char **av) {
    (void)ac; (void)av;
    printf("  PID TTY      STAT TIME COMMAND\n");
    printf("    1 ?        S    0:01 init\n");
    return 0;
}

int cmd_kill(int ac, char **av) {
    if (ac < 2) {
        fprintf(stderr, "kill: usage: kill pid\n");
        return 1;
    }
    int sig = 15;
    int idx = 1;
    if (ac >= 3 && strcmp(av[1], "-s") == 0) {
        sig = atoi(av[2]);
        idx = 3;
    }
    if (idx >= ac) {
        fprintf(stderr, "kill: missing pid\n");
        return 1;
    }
    pid_t pid = atoi(av[idx]);
    if (kill(pid, sig) != 0) {
        perror("kill");
        return 1;
    }
    return 0;
}

int cmd_clear(int ac, char **av) {
    (void)ac; (void)av;
    printf("\033[2J\033[H");
    return 0;
}

int cmd_whoami(int ac, char **av) {
    (void)ac; (void)av;
    printf("root\n");
    return 0;
}

int cmd_uname(int ac, char **av) {
    (void)ac;
    if (ac < 2 || strcmp(av[1], "-a") == 0 || strcmp(av[1], "-s") == 0) {
        printf("DevilCore 2.0.0 devilcore #1 SMP x86_64 GNU/Linux\n");
    } else if (strcmp(av[1], "-m") == 0) {
        printf("x86_64\n");
    } else if (strcmp(av[1], "-n") == 0) {
        printf("devilcore\n");
    }
    return 0;
}

int cmd_date(int ac, char **av) {
    (void)ac; (void)av;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    printf("%04d-%02d-%02d %02d:%02d:%02d\n", 
           tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
           tm->tm_hour, tm->tm_min, tm->tm_sec);
    return 0;
}

int cmd_df(int ac, char **av) {
    (void)ac; (void)av;
    printf("Filesystem     1K-blocks      Used Available Use%% Mounted on\n");
    printf("/dev/sda1        1048576     10240    1036288   1%% /\n");
    return 0;
}

int cmd_free(int ac, char **av) {
    (void)ac; (void)av;
    printf("             total       used       free     shared    buffers     cached\n");
    printf("Mem:        262144       8192     253952          0       1024     2048\n");
    printf("Swap:            0          0          0\n");
    return 0;
}

int cmd_ifconfig(int ac, char **av) {
    (void)ac; (void)av;
    printf("eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n");
    printf("        inet 192.168.1.100  netmask 255.255.255.0  broadcast 192.168.1.255\n");
    printf("        ether 52:54:00:12:34:56  txqueuelen 1000  (Ethernet)\n\n");
    printf("lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536\n");
    printf("        inet 127.0.0.1  netmask 255.0.0.0\n");
    return 0;
}

int cmd_ping(int ac, char **av) {
    if (ac < 2) {
        fprintf(stderr, "ping: usage: ping host\n");
        return 1;
    }
    printf("PING %s (192.168.1.1) 56(84) bytes of data.\n", av[1]);
    for (int i = 0; i < 4; i++) {
        printf("64 bytes from 192.168.1.1: icmp_seq=%d ttl=64 time=0.1 ms\n", i + 1);
        sleep(1);
    }
    printf("\n--- %s ping statistics ---\n", av[1]);
    printf("4 packets transmitted, 4 received, 0%% packet loss\n");
    return 0;
}

int cmd_help(int ac, char **av) {
    (void)ac; (void)av;
    printf("DevilShell v2.0 - DevilCore Command Interpreter\n\n");
    printf("Built-in commands:\n");
    printf("  cd, pwd, echo, exit, help, clear\n");
    printf("  ls, cat, cp, mv, rm, mkdir\n");
    printf("  ps, kill, uname, date, df, free\n");
    printf("  ifconfig, ping\n");
    return 0;
}

int cmd_exit(int ac, char **av) {
    int code = (ac > 1) ? atoi(av[1]) : 0;
    exit(code);
    return 0;
}

typedef struct {
    const char *name;
    int (*func)(int ac, char **av);
} builtin_t;

builtin_t builtins[] = {
    {"cd", cmd_cd},
    {"pwd", cmd_pwd},
    {"echo", cmd_echo},
    {"ls", cmd_ls},
    {"cat", cmd_cat},
    {"mkdir", cmd_mkdir},
    {"rm", cmd_rm},
    {"cp", cmd_cp},
    {"mv", cmd_mv},
    {"ps", cmd_ps},
    {"kill", cmd_kill},
    {"clear", cmd_clear},
    {"whoami", cmd_whoami},
    {"uname", cmd_uname},
    {"date", cmd_date},
    {"df", cmd_df},
    {"free", cmd_free},
    {"ifconfig", cmd_ifconfig},
    {"ping", cmd_ping},
    {"help", cmd_help},
    {"exit", cmd_exit},
    {"quit", cmd_exit},
    {NULL, NULL}
};

int execute_builtin(int ac, char **av) {
    for (int i = 0; builtins[i].name; i++) {
        if (strcmp(av[0], builtins[i].name) == 0) {
            return builtins[i].func(ac, av);
        }
    }
    return -1;
}

int execute_program(int ac, char **av) {
    (void)ac;
    char *path = NULL;
    char *path_env = getenv("PATH");
    if (!path_env) path_env = "/bin:/usr/bin:/sbin:/usr/sbin";
    char path_copy[4096];
    strcpy(path_copy, path_env);
    char *dir = strtok(path_copy, ":");
    while (dir) {
        static char buf[4096];
        snprintf(buf, sizeof(buf), "%s/%s", dir, av[0]);
        if (access(buf, X_OK) == 0) {
            path = buf;
            break;
        }
        dir = strtok(NULL, ":");
    }
    if (!path) {
        fprintf(stderr, "%s: command not found\n", av[0]);
        return 127;
    }
    pid_t pid = fork();
    if (pid == 0) {
        execv(path, av);
        perror(av[0]);
        exit(127);
    } else {
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
    return 0;
}

int execute(int ac, char **av) {
    if (!av[0]) return 0;
    int result = execute_builtin(ac, av);
    if (result != -1) return result;
    return execute_program(ac, av);
}

char **parse_args(char *line, int *ac) {
    static char *args[256];
    char buf[8192];
    int pos = 0, count = 0;
    int in_word = 0;
    for (int i = 0; line[i]; i++) {
        if (line[i] == ' ' || line[i] == '\t') {
            if (in_word) {
                buf[pos] = 0;
                args[count++] = strdup(buf);
                pos = 0;
                in_word = 0;
            }
        } else {
            if (!in_word && line[i] != '#') in_word = 1;
            if (pos < 8191) buf[pos++] = line[i];
        }
    }
    if (in_word && pos > 0) {
        buf[pos] = 0;
        args[count++] = strdup(buf);
    }
    args[count] = NULL;
    *ac = count;
    return args;
}

char *read_line(void) {
    static char line[8192];
    int pos = 0;
    printf("\e[1;32mDevilCore\e[0m:\e[1;34m%s\e[0m$ ", cwd);
    fflush(stdout);
    while (1) {
        int c = getchar();
        if (c == '\n') { line[pos] = 0; return line; }
        else if ((c == 127 || c == 8) && pos > 0) { pos--; printf("\b \b"); }
        else if (c >= 32 && pos < 8191) { line[pos++] = c; putchar(c); }
    }
}

void add_history(char *line) {
    if (line[0] && (history_count == 0 || strcmp(history[history_count - 1], line))) {
        strncpy(history[history_count++], line, 255);
        if (history_count >= 1000) {
            for (int i = 0; i < 999; i++) strcpy(history[i], history[i + 1]);
            history_count = 999;
        }
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("DevilShell v2.0 - DevilCore Command Interpreter\n");
    printf("Type 'help' for available commands\n\n");
    getcwd(cwd, sizeof(cwd));
    strcpy(home, "/root");
    setenv("PATH", "/bin:/usr/bin:/sbin:/usr/sbin", 1);
    setenv("HOME", home, 1);
    while (1) {
        char *line = read_line();
        if (!line[0]) continue;
        add_history(line);
        int ac;
        char **av = parse_args(line, &ac);
        execute(ac, av);
    }
    return 0;
}