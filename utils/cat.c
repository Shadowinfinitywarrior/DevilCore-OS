#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: cat <file>...\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        FILE *f = fopen(argv[i], "r");
        if (!f) {
            fprintf(stderr, "cat: %s: No such file\n", argv[i]);
            continue;
        }
        
        char buf[4096];
        while (fgets(buf, sizeof(buf), f)) {
            printf("%s", buf);
        }
        fclose(f);
    }
    
    return 0;
}