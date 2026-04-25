#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>

int main(int argc, char **argv) {
    printf("  PID TTY      STAT TIME CMD\n");
    
    DIR *dir = opendir("/proc");
    if (!dir) {
        printf("    1 ?        S    0:01 init\n");
        return 0;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
            char path[256];
            snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);
            FILE *f = fopen(path, "r");
            if (f) {
                char cmd[256] = {0};
                fread(cmd, 1, sizeof(cmd)-1, f);
                fclose(f);
                
                if (cmd[0]) {
                    printf(" %5s ?        S    0:00 %s\n", entry->d_name, cmd);
                }
            }
        }
    }
    closedir(dir);
    
    return 0;
}