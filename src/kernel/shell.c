#include "shell.h"
#include "framebuffer.h"
#include <string.h>
#include "vfs.h"
#include "memory.h"
#include "timer.h"
#include "kprintf.h"
#include "wm.h"

extern struct wm_desktop *desktop;

#define SHELL_MAX_LINE 256
#define SHELL_MAX_HISTORY 20

struct shell_data {
    char current_line[SHELL_MAX_LINE];
    uint32_t line_pos;
    char history[SHELL_MAX_HISTORY][SHELL_MAX_LINE];
    uint32_t history_count;
    char output[30][128]; // Terminal output buffer
    uint32_t output_row;
    uint64_t last_blink;
    uint8_t cursor_visible;
    uint8_t needs_redraw;  // Flag to track if content changed
};

static void shell_print(struct shell_data *data, const char *text) {
    if (data == NULL || text == NULL) return;
    
    if (data->output_row >= 28) {
        // Simple scroll - shift all lines up
        for (int i = 0; i < 27; ++i) {
            memcpy(data->output[i], data->output[i+1], 128);
        }
        memset(data->output[27], 0, 128);
        data->output_row = 27;
    }
    // Safely copy text to output buffer
    strncpy(data->output[data->output_row], text, 127);
    data->output[data->output_row][127] = '\0';
    data->output_row++;
    data->needs_redraw = 1;  // Content changed, needs redraw
}

static char current_dir[256] = "/";

static void shell_execute(struct wm_window *win, const char *cmd) {
    struct shell_data *data = (struct shell_data *)win->widget.data;
    char buf[256];
    
    // Echo command with prompt
    sprintf(buf, "root@devilcore:%s# %s", current_dir, cmd);
    shell_print(data, buf);

    if (strcmp(cmd, "ls") == 0 || strncmp(cmd, "ls ", 3) == 0) {
        const char *path = (strlen(cmd) > 3) ? cmd + 3 : current_dir;
        shell_print(data, "total 24");
        shell_print(data, "drwxr-xr-x 2 root root 4096 .");
        shell_print(data, "drwxr-xr-x 2 root root 4096 ..");
        shell_print(data, "drwxr-xr-x 2 root root 4096 bin");
        shell_print(data, "drwxr-xr-x 2 root root 4096 etc");
        shell_print(data, "drwxr-xr-x 2 root root 4096 dev");
        shell_print(data, "drwxr-xr-x 2 root root 4096 tmp");
    } else if (strcmp(cmd, "help") == 0) {
        shell_print(data, "DevilCore Shell v0.3 - Command Reference");
        shell_print(data, "========================================");
        shell_print(data, "");
        shell_print(data, "FILE OPERATIONS:");
        shell_print(data, "  ls          - List directory contents");
        shell_print(data, "  cd <dir>    - Change directory");
        shell_print(data, "  pwd         - Print working directory");
        shell_print(data, "  cat <file>  - Display file contents");
        shell_print(data, "  touch <f>   - Create empty file");
        shell_print(data, "  mkdir <d>   - Create directory");
        shell_print(data, "  rm <file>   - Remove file");
        shell_print(data, "  rmdir <d>   - Remove directory");
        shell_print(data, "  cp <src> <dst> - Copy file");
        shell_print(data, "  mv <src> <dst> - Move/rename file");
        shell_print(data, "  chmod <mode> <file> - Change permissions");
        shell_print(data, "  chown <user> <file> - Change owner");
        shell_print(data, "");
        shell_print(data, "SYSTEM INFO:");
        shell_print(data, "  mem         - Show memory information");
        shell_print(data, "  free        - Show memory status");
        shell_print(data, "  top         - Show system processes");
        shell_print(data, "  ps          - List running processes");
        shell_print(data, "  df          - Show disk free space");
        shell_print(data, "  du          - Show directory usage");
        shell_print(data, "  uptime      - Show system uptime");
        shell_print(data, "  date        - Show system date/time");
        shell_print(data, "  dmesg       - Kernel messages");
        shell_print(data, "  uname [-a]  - Show system info");
        shell_print(data, "");
        shell_print(data, "USER INFO:");
        shell_print(data, "  whoami      - Show current user");
        shell_print(data, "  id          - Show user ID info");
        shell_print(data, "  hostname    - Show system hostname");
        shell_print(data, "  env         - Show environment variables");
        shell_print(data, "  groups      - Show user groups");
        shell_print(data, "  w           - Who is logged in");
        shell_print(data, "");
        shell_print(data, "NETWORK:");
        shell_print(data, "  ifconfig    - Show network config");
        shell_print(data, "  netstat     - Show network connections");
        shell_print(data, "  ping <host> - Ping a host");
        shell_print(data, "  arp         - Show ARP table");
        shell_print(data, "");
        shell_print(data, "HARDWARE:");
        shell_print(data, "  lspci       - List PCI devices");
        shell_print(data, "  lsusb       - List USB devices");
        shell_print(data, "  lshw        - List hardware");
        shell_print(data, "");
        shell_print(data, "UTILITIES:");
        shell_print(data, "  echo <text> - Print text to screen");
        shell_print(data, "  cal         - Show calendar");
        shell_print(data, "  calc <expr> - Calculator");
        shell_print(data, "  clear       - Clear terminal screen");
        shell_print(data, "  history     - Command history");
        shell_print(data, "  alias       - Show command aliases");
        shell_print(data, "");
        shell_print(data, "FUN COMMANDS:");
        shell_print(data, "  neofetch    - System info with logo");
        shell_print(data, "  cowsay      - Talking cow");
        shell_print(data, "  fortune     - Random quote");
        shell_print(data, "  matrix      - Matrix reference");
        shell_print(data, "  weather     - Show weather");
        shell_print(data, "  figlet      - ASCII art text");
        shell_print(data, "  sl          - Steam locomotive");
        shell_print(data, "  motd        - Message of the day");
        shell_print(data, "");
        shell_print(data, "SYSTEM CONTROL:");
        shell_print(data, "  version     - Show OS version");
        shell_print(data, "  reboot      - Reboot system");
        shell_print(data, "  shutdown    - Shutdown system");
        shell_print(data, "  poweroff    - Power off system");
        shell_print(data, "");
        shell_print(data, "Type 'help <cmd>' for detailed help on any command.");
    } else if (strncmp(cmd, "help ", 5) == 0) {
        const char *topic = cmd + 5;
        if (strcmp(topic, "ls") == 0) {
            shell_print(data, "ls [dir] - List directory contents");
            shell_print(data, "Shows files and folders with permissions,");
            shell_print(data, "owner, size, and modification date.");
        } else if (strcmp(topic, "cd") == 0) {
            shell_print(data, "cd <dir> - Change directory");
            shell_print(data, "Navigate to a different folder.");
            shell_print(data, "Use 'cd ..' to go up, 'cd /' for root.");
        } else if (strcmp(topic, "pwd") == 0) {
            shell_print(data, "pwd - Print working directory");
            shell_print(data, "Shows the full path of current folder.");
        } else if (strcmp(topic, "cat") == 0) {
            shell_print(data, "cat <file> - Display file contents");
            shell_print(data, "Reads and outputs entire file to screen.");
        } else if (strcmp(topic, "echo") == 0) {
            shell_print(data, "echo <text> - Print text to screen");
            shell_print(data, "Displays the text you type.");
        } else if (strcmp(topic, "touch") == 0) {
            shell_print(data, "touch <file> - Create empty file");
            shell_print(data, "Creates new file or updates timestamp.");
        } else if (strcmp(topic, "mkdir") == 0) {
            shell_print(data, "mkdir <dir> - Create directory");
            shell_print(data, "Makes a new folder in current location.");
        } else if (strcmp(topic, "rm") == 0) {
            shell_print(data, "rm <file> - Remove file");
            shell_print(data, "Permanently deletes a file. Use with care!");
        } else if (strcmp(topic, "rmdir") == 0) {
            shell_print(data, "rmdir <dir> - Remove directory");
            shell_print(data, "Deletes an empty folder.");
        } else if (strcmp(topic, "clear") == 0) {
            shell_print(data, "clear - Clear terminal screen");
            shell_print(data, "Wipes all text and resets to top.");
        } else if (strcmp(topic, "ps") == 0) {
            shell_print(data, "ps - List running processes");
            shell_print(data, "Shows PID, TTY, CPU time, and command.");
        } else if (strcmp(topic, "mem") == 0) {
            shell_print(data, "mem - Show memory information");
            shell_print(data, "Displays total, used, and free memory.");
        } else if (strcmp(topic, "free") == 0) {
            shell_print(data, "free - Show memory status");
            shell_print(data, "Human-readable memory usage display.");
        } else if (strcmp(topic, "top") == 0) {
            shell_print(data, "top - Show system processes");
            shell_print(data, "Live process list sorted by CPU usage.");
        } else if (strcmp(topic, "df") == 0) {
            shell_print(data, "df - Show disk free space");
            shell_print(data, "Filesystem usage and available space.");
        } else if (strcmp(topic, "du") == 0) {
            shell_print(data, "du - Show directory usage");
            shell_print(data, "Disk space used by files/folders.");
        } else if (strcmp(topic, "uptime") == 0) {
            shell_print(data, "uptime - Show system uptime");
            shell_print(data, "How long system has been running.");
        } else if (strcmp(topic, "date") == 0) {
            shell_print(data, "date - Show system date/time");
            shell_print(data, "Current date and time in system format.");
        } else if (strcmp(topic, "whoami") == 0) {
            shell_print(data, "whoami - Show current user");
            shell_print(data, "Prints your username (root).");
        } else if (strcmp(topic, "uname") == 0) {
            shell_print(data, "uname [-a] - Show system info");
            shell_print(data, "Kernel name, version, architecture.");
        } else if (strcmp(topic, "id") == 0) {
            shell_print(data, "id - Show user ID info");
            shell_print(data, "User and group IDs with memberships.");
        } else if (strcmp(topic, "hostname") == 0) {
            shell_print(data, "hostname - Show system hostname");
            shell_print(data, "Computer name on the network.");
        } else if (strcmp(topic, "env") == 0) {
            shell_print(data, "env - Show environment variables");
            shell_print(data, "All shell environment settings.");
        } else if (strcmp(topic, "cal") == 0) {
            shell_print(data, "cal - Show calendar");
            shell_print(data, "Monthly calendar view.");
        } else if (strcmp(topic, "ifconfig") == 0) {
            shell_print(data, "ifconfig - Show network config");
            shell_print(data, "Interface IP, MAC, packets, errors.");
        } else if (strcmp(topic, "netstat") == 0) {
            shell_print(data, "netstat - Show network connections");
            shell_print(data, "Active sockets and listening ports.");
        } else if (strcmp(topic, "lspci") == 0) {
            shell_print(data, "lspci - List PCI devices");
            shell_print(data, "All PCI/PCIe hardware on bus.");
        } else if (strcmp(topic, "lsusb") == 0) {
            shell_print(data, "lsusb - List USB devices");
            shell_print(data, "Connected USB devices and hubs.");
        } else if (strcmp(topic, "neofetch") == 0) {
            shell_print(data, "neofetch - System info with logo");
            shell_print(data, "Pretty system information display.");
        } else if (strcmp(topic, "cowsay") == 0) {
            shell_print(data, "cowsay - Talking cow");
            shell_print(data, "ASCII art cow says something fun.");
        } else if (strcmp(topic, "fortune") == 0) {
            shell_print(data, "fortune - Random quote");
            shell_print(data, "Displays random wisdom or humor.");
        } else if (strcmp(topic, "matrix") == 0) {
            shell_print(data, "matrix - Matrix reference");
            shell_print(data, "Wake up, Neo...");
        } else if (strcmp(topic, "weather") == 0) {
            shell_print(data, "weather - Show weather");
            shell_print(data, "Current conditions (mock data).");
        } else if (strcmp(topic, "calc") == 0) {
            shell_print(data, "calc <expr> - Calculator");
            shell_print(data, "Simple arithmetic: calc 2+2, calc 10*5");
        } else if (strcmp(topic, "figlet") == 0) {
            shell_print(data, "figlet - ASCII art text");
            shell_print(data, "Renders large ASCII text banner.");
        } else if (strcmp(topic, "sl") == 0) {
            shell_print(data, "sl - Steam locomotive");
            shell_print(data, "Cures bad 'ls' typos. Displays train.");
        } else if (strcmp(topic, "motd") == 0) {
            shell_print(data, "motd - Message of the day");
            shell_print(data, "Shows welcome message and tips.");
        } else if (strcmp(topic, "alias") == 0) {
            shell_print(data, "alias - Show command aliases");
            shell_print(data, "Lists shortcut commands like ll, la.");
        } else if (strcmp(topic, "ll") == 0) {
            shell_print(data, "ll - Detailed list (alias)");
            shell_print(data, "Shows all files with permissions.");
        } else if (strcmp(topic, "la") == 0) {
            shell_print(data, "la - List all files (alias)");
            shell_print(data, "Shows hidden files too.");
        } else if (strcmp(topic, "w") == 0) {
            shell_print(data, "w - Who is logged in");
            shell_print(data, "Shows current users and activity.");
        } else if (strcmp(topic, "groups") == 0) {
            shell_print(data, "groups - Show user groups");
            shell_print(data, "Lists all groups you belong to.");
        } else if (strcmp(topic, "dmesg") == 0) {
            shell_print(data, "dmesg - Kernel messages");
            shell_print(data, "Displays kernel ring buffer messages.");
        } else if (strcmp(topic, "grep") == 0) {
            shell_print(data, "grep <pattern> - Search text");
            shell_print(data, "Finds matching text in files.");
        } else if (strcmp(topic, "wc") == 0) {
            shell_print(data, "wc <file> - Word count");
            shell_print(data, "Counts lines, words, and bytes.");
        } else if (strcmp(topic, "reboot") == 0) {
            shell_print(data, "reboot - Restart system");
            shell_print(data, "Restarts the computer immediately.");
        } else if (strcmp(topic, "shutdown") == 0) {
            shell_print(data, "shutdown - Power off system");
            shell_print(data, "Safely powers down the computer.");
        } else if (strcmp(topic, "version") == 0) {
            shell_print(data, "version - Show OS version");
            shell_print(data, "DevilCore OS version information.");
        } else if (strcmp(topic, "cp") == 0) {
            shell_print(data, "cp <src> <dst> - Copy file");
            shell_print(data, "Copies a file from source to destination.");
        } else if (strcmp(topic, "mv") == 0) {
            shell_print(data, "mv <src> <dst> - Move/rename file");
            shell_print(data, "Moves or renames a file.");
        } else if (strcmp(topic, "chmod") == 0) {
            shell_print(data, "chmod <mode> <file> - Change permissions");
            shell_print(data, "Sets file permissions (e.g., chmod 755 file).");
        } else if (strcmp(topic, "chown") == 0) {
            shell_print(data, "chown <user> <file> - Change owner");
            shell_print(data, "Changes file owner to specified user.");
        } else if (strcmp(topic, "ping") == 0) {
            shell_print(data, "ping <host> - Ping a host");
            shell_print(data, "Sends ICMP packets to test connectivity.");
        } else if (strcmp(topic, "arp") == 0) {
            shell_print(data, "arp - Show ARP table");
            shell_print(data, "Displays IP-to-MAC address mappings.");
        } else if (strcmp(topic, "lshw") == 0) {
            shell_print(data, "lshw - List hardware");
            shell_print(data, "Detailed hardware information.");
        } else if (strcmp(topic, "history") == 0) {
            shell_print(data, "history - Command history");
            shell_print(data, "Shows previously executed commands.");
        } else if (strcmp(topic, "poweroff") == 0) {
            shell_print(data, "poweroff - Power off system");
            shell_print(data, "Shuts down and powers off the system.");
        } else {
            sprintf(buf, "No help for '%s'. Try 'help' for list.", topic);
            shell_print(data, buf);
        }
    } else if (strcmp(cmd, "whoami") == 0) {
        shell_print(data, "root");
    } else if (strcmp(cmd, "pwd") == 0) {
        shell_print(data, current_dir);
    } else if (strncmp(cmd, "cd ", 3) == 0) {
        const char *new_dir = cmd + 3;
        if (strcmp(new_dir, "/") == 0) {
            strcpy(current_dir, "/");
        } else if (strcmp(new_dir, "..") == 0) {
            // Go up one directory
            int len = strlen(current_dir);
            if (len > 1) {
                while (len > 1 && current_dir[len-1] == '/') len--;
                while (len > 0 && current_dir[len-1] != '/') len--;
                current_dir[len] = '\0';
                if (len == 0) strcpy(current_dir, "/");
            }
        } else {
            // Append directory
            int len = strlen(current_dir);
            if (len > 1 && current_dir[len-1] != '/') {
                strcat(current_dir, "/");
            }
            strcat(current_dir, new_dir);
        }
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        shell_print(data, cmd + 5);
    } else if (strcmp(cmd, "uname") == 0) {
        shell_print(data, "DevilCore");
    } else if (strcmp(cmd, "uname -a") == 0) {
        shell_print(data, "DevilCore OS 0.1 x86_64 DevilCore devilcore");
    } else if (strcmp(cmd, "ps") == 0) {
        shell_print(data, "  PID TTY          TIME CMD");
        shell_print(data, "    1 ?        00:00:01 init");
        shell_print(data, "    2 ?        00:00:05 wm");
        shell_print(data, "    3 tty1     00:00:00 shell");
    } else if (strcmp(cmd, "mem") == 0) {
        uint32_t total = pmm_total_pages();
        uint32_t free = pmm_free_pages();
        uint32_t used = total - free;
        uint32_t total_mb = (total * 4096) / (1024 * 1024);
        uint32_t free_mb = (free * 4096) / (1024 * 1024);
        uint32_t used_mb = (used * 4096) / (1024 * 1024);
        
        shell_print(data, "Memory Information:");
        sprintf(buf, "  Total: %u MB (%u pages)", total_mb, total);
        shell_print(data, buf);
        sprintf(buf, "  Used:  %u MB (%u pages)", used_mb, used);
        shell_print(data, buf);
        sprintf(buf, "  Free:  %u MB (%u pages)", free_mb, free);
        shell_print(data, buf);
    } else if (strcmp(cmd, "time") == 0) {
        uint64_t ticks = timer_ticks();
        uint32_t sec = (uint32_t)(ticks / 100);
        uint32_t min = sec / 60;
        uint32_t hr = min / 60;
        sprintf(buf, "Uptime: %02u:%02u:%02u (%u ticks)", hr % 24, min % 60, sec % 60, (uint32_t)ticks);
        shell_print(data, buf);
    } else if (strcmp(cmd, "date") == 0) {
        shell_print(data, "System date: 2026-04-30");
        shell_print(data, "Note: Real-time clock not yet implemented");
    } else if (strcmp(cmd, "version") == 0) {
        shell_print(data, "DevilCore OS v0.1");
        shell_print(data, "Built: April 2026");
        shell_print(data, "Architecture: x86_64");
        shell_print(data, "Kernel: 0.1.0-dev");
    } else if (strcmp(cmd, "reboot") == 0) {
        shell_print(data, "Rebooting system...");
        wm_draw_window(win);
        fb_flip();
        // Wait a bit then reboot via keyboard controller
        for (volatile int i = 0; i < 1000000; i++);
        __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x02), "Nd"((uint16_t)0x92));
        __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x00), "Nd"((uint16_t)0x92));
    } else if (strcmp(cmd, "shutdown") == 0) {
        shell_print(data, "Shutting down...");
        shell_print(data, "It is now safe to turn off your computer.");
        wm_draw_window(win);
        fb_flip();
        // Halt the CPU
        __asm__ volatile ("cli; hlt");
    } else if (strcmp(cmd, "clear") == 0) {
        data->output_row = 0;
        for (int i = 0; i < 30; ++i) data->output[i][0] = '\0';
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        const char *filename = cmd + 4;
        struct vfs_file file;
        if (vfs_open(&file, filename, 0) == 0) {
            char buf[128];
            size_t rb = 0;
            if (vfs_read(&file, buf, 127, &rb) == 0) {
                buf[rb] = '\0';
                shell_print(data, buf);
            }
            vfs_close(&file);
        } else {
            sprintf(buf, "cat: %s: No such file or directory", filename);
            shell_print(data, buf);
        }
    } else if (strncmp(cmd, "touch ", 6) == 0) {
        const char *filename = cmd + 6;
        struct vfs_file file;
        // Try to create file
        if (vfs_create(filename, 0) == 0) {
            sprintf(buf, "Created: %s", filename);
            shell_print(data, buf);
        } else {
            sprintf(buf, "touch: cannot create '%s'", filename);
            shell_print(data, buf);
        }
    } else if (strncmp(cmd, "mkdir ", 6) == 0) {
        const char *dirname = cmd + 6;
        sprintf(buf, "mkdir: cannot create directory '%s': Not implemented", dirname);
        shell_print(data, buf);
    } else if (strncmp(cmd, "rm ", 3) == 0) {
        const char *filename = cmd + 3;
        sprintf(buf, "rm: cannot remove '%s': Not implemented", filename);
        shell_print(data, buf);
    } else if (strncmp(cmd, "rmdir ", 6) == 0) {
        const char *dirname = cmd + 6;
        sprintf(buf, "rmdir: cannot remove '%s': Not implemented", dirname);
        shell_print(data, buf);
    } else if (strcmp(cmd, "df") == 0) {
        shell_print(data, "Filesystem     1K-blocks    Used Available Use% Mounted on");
        shell_print(data, "rootfs           4096000 1024000   3072000  25% /");
        shell_print(data, "devfs                  0       0         0   0% /dev");
        shell_print(data, "tmpfs             512000    1024    510976   1% /tmp");
    } else if (strcmp(cmd, "du") == 0) {
        shell_print(data, "4       ./bin");
        shell_print(data, "12      ./etc");
        shell_print(data, "256     ./dev");
        shell_print(data, "48      ./tmp");
        shell_print(data, "320     .");
    } else if (strcmp(cmd, "env") == 0) {
        shell_print(data, "PATH=/bin:/usr/bin:/sbin");
        shell_print(data, "HOME=/root");
        shell_print(data, "USER=root");
        shell_print(data, "SHELL=/bin/sh");
        shell_print(data, "TERM=devilcore-terminal");
        shell_print(data, "OS=DevilCore");
        shell_print(data, "EDITOR=te");
        shell_print(data, "PAGER=cat");
    } else if (strcmp(cmd, "history") == 0) {
        shell_print(data, "Command history not yet implemented.");
    } else if (strcmp(cmd, "cal") == 0) {
        shell_print(data, "     May 2026");
        shell_print(data, "Su Mo Tu We Th Fr Sa");
        shell_print(data, "                1  2");
        shell_print(data, " 3  4  5  6  7  8  9");
        shell_print(data, "10 11 12 13 14 15 16");
        shell_print(data, "17 18 19 20 21 22 23");
        shell_print(data, "24 25 26 27 28 29 30");
        shell_print(data, "31");
    } else if (strcmp(cmd, "uptime") == 0) {
        uint64_t ticks = timer_ticks();
        uint32_t sec = (uint32_t)(ticks / 100);
        uint32_t min = sec / 60;
        uint32_t hr = min / 60;
        uint32_t days = hr / 24;
        sprintf(buf, " %02u:%02u:%02u up %u days, %02u:%02u, 1 user, load average: 0.00, 0.00, 0.00",
                hr % 24, min % 60, sec % 60, days, hr % 24, min % 60);
        shell_print(data, buf);
    } else if (strcmp(cmd, "hostname") == 0) {
        shell_print(data, "devilcore");
    } else if (strcmp(cmd, "id") == 0) {
        shell_print(data, "uid=0(root) gid=0(root) groups=0(root),1(bin),2(daemon)");
    } else if (strcmp(cmd, "ifconfig") == 0) {
        shell_print(data, "eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500");
        shell_print(data, "        inet 192.168.1.100  netmask 255.255.255.0  broadcast 192.168.1.255");
        shell_print(data, "        ether 52:54:00:12:34:56  txqueuelen 1000  (Ethernet)");
        shell_print(data, "        RX packets 1024  bytes 1048576 (1.0 MiB)");
        shell_print(data, "        TX packets 512  bytes 524288 (512 KiB)");
    } else if (strcmp(cmd, "netstat") == 0) {
        shell_print(data, "Active Internet connections");
        shell_print(data, "Proto Recv-Q Send-Q Local Address    Foreign Address   State");
        shell_print(data, "tcp        0      0 0.0.0.0:22       0.0.0.0:*         LISTEN");
        shell_print(data, "tcp        0      0 0.0.0.0:80       0.0.0.0:*         LISTEN");
    } else if (strcmp(cmd, "free") == 0) {
        uint32_t total = pmm_total_pages();
        uint32_t free = pmm_free_pages();
        uint32_t used = total - free;
        uint32_t total_mb = (total * 4) / 1024;
        uint32_t free_mb = (free * 4) / 1024;
        uint32_t used_mb = (used * 4) / 1024;
        shell_print(data, "              total       used       free");
        sprintf(buf, "Mem:      %8u %8u %8u", total_mb, used_mb, free_mb);
        shell_print(data, buf);
        sprintf(buf, "Swap:           0          0          0");
        shell_print(data, buf);
    } else if (strcmp(cmd, "top") == 0) {
        shell_print(data, "PID  USER    PR  NI  VIRT  RES  SHR S %CPU %MEM   TIME+  COMMAND");
        shell_print(data, "  1  root    20   0   512   64   32 R  0.0  0.0   0:00.01 init");
        shell_print(data, "  2  root    20   0  1024  256  128 S  0.0  0.1   0:00.05 wm");
        shell_print(data, "  3  root    20   0   768  128   64 R  0.0  0.0   0:00.00 shell");
    } else if (strcmp(cmd, "lsusb") == 0) {
        shell_print(data, "Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub");
        shell_print(data, "Bus 001 Device 002: ID 046d:c52b Logitech, Inc. Unifying Receiver");
        shell_print(data, "Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub");
    } else if (strcmp(cmd, "lspci") == 0) {
        shell_print(data, "00:00.0 Host bridge: Intel Corporation 440FX - 82441FX PMC");
        shell_print(data, "00:01.0 ISA bridge: Intel Corporation 82371SB PIIX3 ISA");
        shell_print(data, "00:02.0 VGA compatible controller: Device 1234:1111");
        shell_print(data, "00:03.0 Ethernet controller: Realtek RTL8139");
    } else if (strcmp(cmd, "neofetch") == 0) {
        shell_print(data, "         .---.            root@devilcore");
        shell_print(data, "        /     \\           ------------");
        shell_print(data, "       | O   O |          OS: DevilCore OS 0.1");
        shell_print(data, "       |  \\_/  |          Kernel: 0.1.0-dev");
        shell_print(data, "        \\_____/           Uptime: Just now");
        shell_print(data, "          | |             Shell: DevilCore Shell");
        shell_print(data, "          | |             Resolution: 1024x768");
        shell_print(data, "         /   \\            DE: DevilCore WM");
        shell_print(data, "        '     '           WM: Monolithic Framebuffer");
        shell_print(data, "                         Theme: Dark Modern");
        shell_print(data, "                         Icons: PixelArt");
    } else if (strcmp(cmd, "cowsay") == 0) {
        shell_print(data, " _______");
        shell_print(data, "< Moo! >");
        shell_print(data, " -------");
        shell_print(data, "        \\   ^__^");
        shell_print(data, "         \\  (oo)\\_______");
        shell_print(data, "            (__)\\       )\\/\\");
        shell_print(data, "                ||----w |");
        shell_print(data, "                ||     ||");
    } else if (strncmp(cmd, "hexdump ", 8) == 0) {
        shell_print(data, "00000000  7f 45 4c 46 02 01 01 00  00 00 00 00 00 00 00 00  |.ELF............|");
        shell_print(data, "00000010  02 00 3e 00 01 00 00 00  00 00 00 00 00 00 00 00  |..>.............|");
    } else if (strcmp(cmd, "fortune") == 0) {
        const char *fortunes[] = {
            "The only way to do great work is to love what you do. - Steve Jobs",
            "Stay hungry, stay foolish. - Steve Jobs",
            "Talk is cheap. Show me the code. - Linus Torvalds",
            "Software is like sex: it's better when it's free. - Linus Torvalds",
            "Hack the planet!",
            "With great power comes great responsibility."
        };
        int idx = timer_ticks() % 6;
        shell_print(data, fortunes[idx]);
    } else if (strcmp(cmd, "weather") == 0) {
        shell_print(data, "Weather for Unknown Location:");
        shell_print(data, "  Sunny, 72F / 22C");
        shell_print(data, "  Humidity: 45%");
        shell_print(data, "  Wind: 5 mph NW");
    } else if (strcmp(cmd, "matrix") == 0) {
        shell_print(data, "Wake up, Neo...");
        shell_print(data, "The Matrix has you...");
        shell_print(data, "Follow the white rabbit.");
        shell_print(data, "");
        shell_print(data, "01001000 01100101 01101100 01101100 01101111");
    } else if (strncmp(cmd, "calc ", 5) == 0) {
        const char *expr = cmd + 5;
        int a = 0, b = 0, result = 0;
        char op = 0;
        // Simple parser for expressions like "2 + 2" or "10*5"
        // Skip leading spaces
        while (*expr == ' ') expr++;
        // Parse first number
        while (*expr >= '0' && *expr <= '9') {
            a = a * 10 + (*expr - '0');
            expr++;
        }
        // Skip spaces before operator
        while (*expr == ' ') expr++;
        // Get operator
        op = *expr;
        if (op == '+' || op == '-' || op == '*' || op == '/' || op == '%') {
            expr++;
            // Skip spaces after operator
            while (*expr == ' ') expr++;
            // Parse second number
            while (*expr >= '0' && *expr <= '9') {
                b = b * 10 + (*expr - '0');
                expr++;
            }
            // Calculate
            switch (op) {
                case '+': result = a + b; break;
                case '-': result = a - b; break;
                case '*': result = a * b; break;
                case '/': result = (b != 0) ? a / b : 0; break;
                case '%': result = (b != 0) ? a % b : 0; break;
                default: result = 0; break;
            }
            sprintf(buf, "%d %c %d = %d", a, op, b, result);
            shell_print(data, buf);
        } else {
            shell_print(data, "Usage: calc <num> <op> <num>");
            shell_print(data, "  calc 2+2     calc 10*5");
            shell_print(data, "  calc 20/4    calc 15%7");
        }
    } else if (strcmp(cmd, "figlet") == 0) {
        shell_print(data, " _       _ _ _               _   ");
        shell_print(data, "| |_ ___| | | |___ ___ _ _ _| |_ ");
        shell_print(data, "|   / -_| | | / _ / _ | '_/ _|  _|");
        shell_print(data, "|_|_\\___|_|_|_\\___\___|_| \\__|\\__|");
    } else if (strcmp(cmd, "sl") == 0) {
        shell_print(data, "      ====        ________                ___________");
        shell_print(data, "  _D _|  |_______/        \\_____I______ /             \___");
        shell_print(data, "   |(_)---  |   H\\________/ |   |        =|___ ___|   _______");
        shell_print(data, "   /     |  |   H  |  |     |   |         ||_| |_||   _|     ");
        shell_print(data, "  |      |  |   H  |__--------------------| [___] | |=|   _____");
        shell_print(data, "  | ________|___H__/__|_____/[][]~\\_______|       |-| ____|");
        shell_print(data, "  |/ |   |-----------I_____I [][] []  D   |=======|____|_____|");
        shell_print(data, "__/ =| o |=-~~\\  /~~\\  /~~\\  /~~\\ ____Y___________|__|______");
    } else if (strcmp(cmd, "motd") == 0) {
        shell_print(data, "Welcome to DevilCore OS v0.2!");
        shell_print(data, "");
        shell_print(data, "This is an educational operating system");
        shell_print(data, "built for learning OS development.");
        shell_print(data, "");
        shell_print(data, "Type 'help' to see available commands.");
        shell_print(data, "Try 'neofetch' for system info or 'cowsay' for fun!");
    } else if (strcmp(cmd, "alias") == 0) {
        shell_print(data, "Aliases available:");
        shell_print(data, "  ll = ls -la   (detailed list)");
        shell_print(data, "  la = ls -a    (all files)");
        shell_print(data, "  h  = history  (command history)");
    } else if (strcmp(cmd, "ll") == 0) {
        shell_print(data, "total 24");
        shell_print(data, "drwxr-xr-x 2 root root 4096 .");
        shell_print(data, "drwxr-xr-x 2 root root 4096 ..");
        shell_print(data, "-rw-r--r-- 1 root root    0 .profile");
        shell_print(data, "drwxr-xr-x 2 root root 4096 bin");
        shell_print(data, "drwxr-xr-x 2 root root 4096 etc");
        shell_print(data, "drwxr-xr-x 2 root root 4096 dev");
        shell_print(data, "drwxr-xr-x 2 root root 4096 tmp");
    } else if (strcmp(cmd, "la") == 0) {
        shell_print(data, ".");
        shell_print(data, "..");
        shell_print(data, ".profile");
        shell_print(data, "bin");
        shell_print(data, "etc");
        shell_print(data, "dev");
        shell_print(data, "tmp");
    } else if (strcmp(cmd, "w") == 0 || strcmp(cmd, "users") == 0) {
        shell_print(data, "USER     TTY      FROM             LOGIN@   IDLE   JCPU   PCPU  WHAT");
        shell_print(data, "root     tty1     -                08:00    0.00s  0.00s  0.00s shell");
    } else if (strcmp(cmd, "groups") == 0) {
        shell_print(data, "root bin daemon sys adm disk wheel floppy dialout tape video");
    } else if (strcmp(cmd, "dmesg") == 0) {
        shell_print(data, "[0.000000] DevilCore OS v0.2 booting...");
        shell_print(data, "[0.001234] Framebuffer initialized");
        shell_print(data, "[0.002345] Window Manager loaded");
        shell_print(data, "[0.003456] PCI devices enumerated");
        shell_print(data, "[0.004567] Shell ready");
        shell_print(data, "[0.005678] System ready for operation");
    } else if (strncmp(cmd, "grep ", 5) == 0) {
        shell_print(data, "grep: simulated search (no actual files)");
        shell_print(data, "Pattern would be searched in files.");
    } else if (strncmp(cmd, "wc ", 3) == 0) {
        shell_print(data, "  10   50  300 test.txt");
    } else if (strcmp(cmd, "clear") == 0) {
        // Clear all output lines
        for (int i = 0; i < 30; i++) {
            memset(data->output[i], 0, 128);
        }
        data->output_row = 0;
        data->needs_redraw = 1;
        shell_print(data, "DevilCore Shell v0.2 (screen cleared)");
    } else if (strlen(cmd) == 0) {
        // Empty command - just show new prompt
    } else {
        sprintf(buf, "%s: command not found", cmd);
        shell_print(data, buf);
        shell_print(data, "Type 'help' for available commands.");
    }
}

void shell_init(struct wm_window *win) {
    struct shell_data *data = kmalloc(sizeof(struct shell_data));
    memset(data, 0, sizeof(struct shell_data));
    win->widget.data = data;
    win->widget.on_event = shell_on_event;
    data->cursor_visible = 1;
    data->last_blink = timer_ticks();
    data->needs_redraw = 1;
    shell_print(data, "DevilCore Shell v0.2 - 40+ commands");
    shell_print(data, "Type 'help' or 'help <cmd>' for info.");
}

void shell_on_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    struct shell_data *sd = (struct shell_data *)win->widget.data;
    
    if (sd == NULL) return;
    
    // Let the default window handler check for title bar buttons first
    if (wm_window_on_event(widget, event)) {
        return; // Event was handled by title bar button
    }
    
    // Ensure window is focused when clicked
    if (event->type == WM_EVENT_MOUSE_PRESS) {
        wm_focus_window(win);
        desktop->needs_redraw = 1;
        return;
    }
    
    if (event->type == WM_EVENT_KEY) {
        char c = event->key;
        sd->needs_redraw = 1;  // Content will change
        if (c == '\n') {
            sd->current_line[sd->line_pos] = '\0';
            shell_execute(win, sd->current_line);
            sd->line_pos = 0;
            sd->current_line[0] = '\0';
        } else if (c == '\b') {
            if (sd->line_pos > 0) sd->line_pos--;
            sd->current_line[sd->line_pos] = '\0';
        } else if (sd->line_pos < SHELL_MAX_LINE - 1) {
            sd->current_line[sd->line_pos++] = c;
            sd->current_line[sd->line_pos] = '\0';
        }
    }
}

// Global for shell cursor blink to sync across all terminals
static uint64_t shell_global_last_blink = 0;
static uint8_t shell_global_cursor_visible = 1;

void shell_update(struct wm_window *win) {
    struct shell_data *sd = (struct shell_data *)win->widget.data;
    if (sd == NULL) return;
    
    uint32_t x = win->widget.x + 8;
    uint32_t y = win->widget.y + 8; // Adjust for within window
    
    // Handle cursor blink globally (not per-shell) to prevent flickering
    uint64_t ticks = timer_ticks();
    uint8_t cursor_changed = 0;
    if (ticks - shell_global_last_blink >= 20) {
        shell_global_last_blink = ticks;
        shell_global_cursor_visible = !shell_global_cursor_visible;
        cursor_changed = 1;
    }
    
    // Only redraw if content changed or cursor blinked
    if (!sd->needs_redraw && !cursor_changed) {
        return;  // Nothing changed, skip redraw
    }
    sd->needs_redraw = 0;  // Clear the flag
    
    // Clear the window content area first (to prevent text ghosting)
    uint32_t bg_color = win->bg_color != 0 ? win->bg_color : 0x00404040;
    fb_fill_rect(x, y, win->widget.width - 16, win->widget.height - 16, bg_color);

    for (uint32_t i = 0; i < sd->output_row; ++i) {
        fb_draw_string(x, y + i * 12, sd->output[i], 0x0000ff00, 0);
    }

    // Draw prompt and current line
    char prompt[SHELL_MAX_LINE + 64];
    int p = 0;
    // Build prompt with current directory
    sprintf(prompt, "root@devilcore:%s# ", current_dir);
    p = strlen(prompt);
    const char *curr = sd->current_line;
    while (*curr && p < SHELL_MAX_LINE + 62) prompt[p++] = *curr++;
    
    // Add blinking cursor (use global state)
    if (shell_global_cursor_visible && win->widget.focused) {
        prompt[p++] = '_';
    }
    prompt[p] = '\0';
    
    fb_draw_string(x, y + sd->output_row * 12, prompt, 0x00ffffff, 0);
}
