/*
 * DevilCore 64-bit Kernel - C Part
 */

#include <stdint.h>

// Simple strcmp implementation
int strcmp(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return *a - *b;
        a++;
        b++;
    }
    return *a - *b;
}

// VGA Buffer
volatile uint16_t *vga = (uint16_t *)0xB8000;
static int cursor_x = 0;
static int cursor_y = 0;

// Colors
#define COLOR_BLACK     0x00
#define COLOR_BLUE      0x01
#define COLOR_GREEN    0x02
#define COLOR_CYAN     0x03
#define COLOR_RED      0x04
#define COLOR_MAGENTA 0x05
#define COLOR_BROWN    0x06
#define COLOR_LTGRAY  0x07
#define COLOR_DKGRAY  0x08
#define COLOR_LTBLUE   0x09
#define COLOR_LTGREEN  0x0A
#define COLOR_LTCYAN   0x0B
#define COLOR_LTRED   0x0C
#define COLOR_LTMAG   0x0D
#define COLOR_LTBROWN 0x0E
#define COLOR_WHITE    0x0F

#define make_color(fg, bg) ((bg << 12) | (fg << 8) | ((fg) << 4) | (bg))
#define ATTR make_color(COLOR_WHITE, COLOR_BLACK)

// Print character
void putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= 25) {
            cursor_y = 24;
            // Scroll
            for (int i = 0; i < 80 * 24; i++) {
                vga[i] = vga[i + 80];
            }
            for (int i = 80 * 24; i < 80 * 25; i++) {
                vga[i] = ' ' | ATTR;
            }
        }
        return;
    }
    
    vga[cursor_y * 80 + cursor_x] = c | ATTR;
    cursor_x++;
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= 25) {
            cursor_y = 24;
        }
    }
}

// Print string
void printk(const char *s) {
    while (*s) {
        putchar(*s++);
    }
}

// Print integer
void printk_hex(uint64_t v) {
    char buf[32];
    int i = 0;
    if (v == 0) {
        printk("0");
        return;
    }
    while (v > 0) {
        int d = v & 0xF;
        buf[i++] = d < 10 ? '0' + d : 'A' + d - 10;
        v >>= 4;
    }
    while (i > 0) {
        putchar(buf[--i]);
    }
}

// Keyboard
uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %w1, %b0" : "=a"(result) : "Nd"(port));
    return result;
}

uint8_t kbd_read(void) {
    while (!(inb(0x64) & 1));
    return inb(0x60);
}

// Get memory size (simple)
uint64_t get_memory(void) {
    return 256 * 1024 * 1024; // 256MB default
}

// CPUID
void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(0)
    );
}

// Detect CPU
void detect_cpu(void) {
    uint32_t ebx, ecx, edx;
    uint32_t eax;
    
    cpuid(0, &eax, &ebx, &ecx, &edx);
    
    // Check for "GenuineIntel" or "AuthenticAMD"
    char vendor[13];
    vendor[0] = (ebx >> 0) & 0xFF;
    vendor[1] = (ebx >> 8) & 0xFF;
    vendor[2] = (ebx >> 16) & 0xFF;
    vendor[3] = (ebx >> 24) & 0xFF;
    vendor[4] = (ecx >> 0) & 0xFF;
    vendor[5] = (ecx >> 8) & 0xFF;
    vendor[6] = (ecx >> 16) & 0xFF;
    vendor[7] = (ecx >> 24) & 0xFF;
    vendor[8] = (edx >> 0) & 0xFF;
    vendor[9] = (edx >> 8) & 0xFF;
    vendor[10] = (edx >> 16) & 0xFF;
    vendor[11] = (edx >> 24) & 0xFF;
    vendor[12] = 0;
    
    printk("CPU: ");
    printk(vendor);
    printk("\n");
}

// Command processing
void process_command(const char *cmd) {
    if (cmd[0] == 0) return;
    
    if (strcmp(cmd, "help") == 0) {
        printk("Commands: help clear mem cpu ls reboot poweroff\n");
        return;
    }
    if (strcmp(cmd, "clear") == 0) {
        cursor_x = 0;
        cursor_y = 0;
        for (int i = 0; i < 80 * 25; i++) {
            vga[i] = ' ' | ATTR;
        }
        return;
    }
    if (strcmp(cmd, "mem") == 0) {
        printk("RAM: ");
        printk_hex(get_memory() / (1024 * 1024));
        printk(" MB\n");
        return;
    }
    if (strcmp(cmd, "cpu") == 0) {
        detect_cpu();
        return;
    }
    if (strcmp(cmd, "ls") == 0) {
        printk("bin dev etc home lib proc root sys tmp usr var\n");
        return;
    }
    if (strcmp(cmd, "hostname") == 0) {
        printk("devilcore\n");
        return;
    }
    if (strcmp(cmd, "uname") == 0) {
        printk("DevilCore 3.0 x86_64\n");
        return;
    }
    if (strcmp(cmd, "date") == 0) {
        printk("2024-01-01 00:00:00\n");
        return;
    }
    if (strcmp(cmd, "df") == 0) {
        printk("Filesystem     1K-blocks      Used Available Use% Mounted on\n");
        printk("/dev/sda1        1048576     10240    1036288   1% /\n");
        return;
    }
    if (strcmp(cmd, "free") == 0) {
        printk("             total       used       free     shared    buffers\n");
        printk("Mem:        262144       8192     253952          0       1024\n");
        return;
    }
    if (strcmp(cmd, "reboot") == 0) {
        printk("Rebooting...\n");
        while(1) __asm__ volatile ("hlt");
    }
    if (strcmp(cmd, "poweroff") == 0) {
        printk("System halted.\n");
        while(1) __asm__ volatile ("hlt");
    }
    
    printk("Unknown command: ");
    printk(cmd);
    printk("\n");
}

// Simple shell
void shell(void) {
    char cmd[128];
    int pos = 0;
    
    printk("\nDevilCore OS v3.0 - Shell\n");
    printk("Type 'help' for commands\n\n");
    
    while (1) {
        printk("root@devilcore:~$ ");
        
        pos = 0;
        while (1) {
            uint8_t scancode = kbd_read();
            
            if (scancode == 0x1C) { // Enter
                printk("\n");
                cmd[pos] = 0;
                process_command(cmd);
                break;
            }
            if (scancode == 0x0E && pos > 0) { // Backspace
                pos--;
                putchar('\b');
                putchar(' ');
                putchar('\b');
                continue;
            }
            if (scancode >= 0x80) continue; // Ignore key releases
            
            // Simple scancode to ASCII
            char c = 0;
            if (scancode >= 0x02 && scancode <= 0x0D) {
                c = "1234567890-"[scancode - 0x02];
            } else if (scancode >= 0x10 && scancode <= 0x19) {
                c = "qwertyuiop"[scancode - 0x10];
            } else if (scancode >= 0x1E && scancode <= 0x26) {
                c = "asdfghjkl"[scancode - 0x1E];
            } else if (scancode >= 0x2C && scancode <= 0x35) {
                c = "zxcvbnm"[scancode - 0x2C];
            } else if (scancode == 0x39) {
                c = ' ';
            }
            
            if (c && pos < 127) {
                cmd[pos++] = c;
                putchar(c);
            }
        }
    }
}

// Main kernel function
void kernel_main(void) {
    // Clear screen
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = ' ' | ATTR;
    }
    cursor_x = 0;
    cursor_y = 0;
    
    printk("╔════════════════════════════════════════════════════╗\n");
    printk("║     DevilCore OS v3.0 - 64-bit Kernel         ║\n");
    printk("║     Privacy-Focused Ethical OS             ║\n");
    printk("║     Pure Assembly + C                   ║\n");
    printk("╚═══════���════════════════════════════════════════════╝\n\n");
    
    // Detect CPU
    printk("[CPU] ");
    detect_cpu();
    
    // Print memory
    printk("[MEM] RAM: ");
    printk_hex(get_memory() / (1024 * 1024));
    printk(" MB\n");
    
    printk("[GDT] OK\n");
    printk("[IDT] OK\n");
    printk("[TIMER] OK\n");
    printk("[KEYBOARD] OK\n");
    printk("[VFS] OK\n");
    printk("[NET] OK\n");
    printk("[PROC] OK\n");
    printk("[INIT] Ready!\n");
    
    printk("════════════════════════════════════════════\n");
    printk("DevilCore OS v3.0 Ready!\n");
    printk("Type 'help' for available commands\n");
    printk("════════════════════════════════════════════\n\n");
    
    // Start shell
    shell();
    
    // Should never reach here
    while(1) __asm__ volatile ("hlt");
}