#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

static void do_ls(const char *path, int show_all, int long_format) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "ls: cannot access '%s'\n", path);
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }
        printf("%s  ", entry->d_name);
    }
    printf("\n");
    closedir(dir);
}

int main(int argc, char **argv) {
    int show_all = 0;
    int long_format = 0;
    const char *path = ".";
    
    int opt;
    while ((opt = getopt(argc, argv, "la")) != -1) {
        switch (opt) {
            case 'a':
                show_all = 1;
                break;
            case 'l':
                long_format = 1;
                break;
            default:
                fprintf(stderr, "Usage: ls [-la] [path]\n");
                return 1;
        }
    }
    
    if (optind < argc) {
        path = argv[optind];
    }
    
    do_ls(path, show_all, long_format);
    return 0;
}