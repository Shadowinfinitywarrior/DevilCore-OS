# apps/settings/settings.c
// System settings

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    const char *name;
    const char *value;
} setting_t;

static setting_t settings[] = {
    {"hostname", "devilcore"},
    {"version", "1.0"},
    {"privacy", "1"},
    {NULL, NULL}
};

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("DevilCore Settings\n");
        for (int i = 0; settings[i].name; i++) {
            printf("%s = %s\n", settings[i].name, settings[i].value);
        }
        return 0;
    }
    
    const char *cmd = argv[1];
    
    if (!strcmp(cmd, "get")) {
        for (int i = 0; settings[i].name; i++) {
            if (!strcmp(settings[i].name, argv[2])) {
                printf("%s\n", settings[i].value);
                return 0;
            }
        }
        return 1;
    }
    
    printf("Usage: settings [get|set] <name> [value]\n");
    return 0;
}