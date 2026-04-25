#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: mkdir <dir>...\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0755) < 0) {
            fprintf(stderr, "mkdir: cannot create directory '%s'\n", argv[i]);
        }
    }
    
    return 0;
}