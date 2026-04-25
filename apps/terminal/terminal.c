# apps/terminal/terminal.c
// Terminal emulator

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

static struct termios orig_term;
static int raw_mode;

void terminal_raw(void) {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    orig_term = t;
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    raw_mode = 1;
}

void terminal_reset(void) {
    if (raw_mode) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
        raw_mode = 0;
    }
}

int main(int argc, char **argv) {
    terminal_raw();
    
    printf("DevilTerm v1.0\n");
    printf("Type 'exit' to quit\n");
    
    char buf[4096];
    while (1) {
        printf("> ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) break;
        system(buf);
    }
    
    terminal_reset();
    return 0;
}