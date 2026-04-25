/*
 * DevilCore OS - Simple Main Function
 * Called from assembly boot code
 */
#include <stdint.h>

// VGA text buffer
volatile uint16_t *vga = (uint16_t *)0xB8000;
static int cursor = 0;

// Simple putchar
void putchar(char c) {
    if (c == '\n' || cursor >= 80 * 25) {
        cursor = (cursor / 80 + 1) * 80;
        if (cursor >= 80 * 25) cursor = 0;
        return;
    }
    vga[cursor++] = c | 0x0F00;
}

// Simple print
void print(const char *s) {
    while (*s) putchar(*s++);
}

// Simple strlen
int strlen(const char *s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

// Simple main
void main(void) {
    print("\n[OK] Kernel loaded successfully!\n\n");
    print("DevilCore OS v3.0 - Running\n");
    print("Type 'help' for commands\n\n");
    
    // Simple shell
    char cmd[64];
    int pos = 0;
    
    while (1) {
        print("root@devilcore:~$ ");
        
        // Read key (simplified - doesn't actually read in this demo)
        print("\n[Demo] System running!\n");
        print("Use 'reboot' or 'poweroff' to test\n\n");
        
        break;
    }
}

// Exit function (for GRUB)
void grub_exit(int code) {
    (void)code;
    __asm__ volatile ("hlt");
}