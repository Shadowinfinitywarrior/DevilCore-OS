# apps/text_editor/editor.c
// Simple text editor

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINES 10000
#define MAX_LINE_LEN 1024

static char *lines[MAX_LINES];
static int line_count;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: editor <file>\n");
        return 1;
    }
    
    printf("DevilEdit v1.0 - Simple Text Editor\n");
    printf("Ctrl-X to save and exit\n");
    printf("Ctrl-C to exit without saving\n");
    
    line_count = 0;
    char buf[MAX_LINE_LEN];
    
    while (fgets(buf, sizeof(buf), stdin)) {
        if (line_count >= MAX_LINES) break;
        lines[line_count++] = strdup(buf);
    }
    
    // Save to file
    FILE *f = fopen(argv[1], "w");
    if (f) {
        for (int i = 0; i < line_count; i++) {
            fputs(lines[i], f);
        }
        fclose(f);
    }
    
    return 0;
}