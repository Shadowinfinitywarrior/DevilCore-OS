# apps/file_manager/file_manager.c
// File manager

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

static void list_dir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        printf("Cannot open %s\n", path);
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        struct stat st;
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
        stat(full, &st);
        
        printf("%c %10ld %s\n",
            S_ISDIR(st.st_mode) ? 'd' : '-',
            (long)st.st_size,
            entry->d_name);
    }
    closedir(dir);
}

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : ".";
    list_dir(path);
    return 0;
}