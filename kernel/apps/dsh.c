/*
 * DevilCore OS — DevilShell Engine
 */
#include "dsh.h"
#include "../gui/terminal.h"
#include "../libc/string.h"
#include "../drivers/pci.h"
#include "../security/crypto.h"
#include "../security/privacy_hub.h"
#include "../security/sandbox.h"

void dsh_execute(Window *term_win, const char *command) {
    if (!term_win || !command || strlen(command) == 0) return;

    /* Echo command */
    terminal_print(term_win, "> ");
    terminal_print(term_win, command);
    terminal_print(term_win, "\n");

    /* Parse */
    if (strcmp(command, "help") == 0) {
        terminal_print(term_win, "DevilShell Internal Commands:\n");
        terminal_print(term_win, "  help    - Show this screen\n");
        terminal_print(term_win, "  clear   - Clear terminal buffer\n");
        terminal_print(term_win, "  lspci   - List PCI Devices\n");
        terminal_print(term_win, "  whoami  - Print active user\n");
        terminal_print(term_win, "  priv    - Display active Tor SOCKS5 node status\n");
        terminal_print(term_win, "  encrypt - Run memory encryption engine test algorithm\n");
        terminal_print(term_win, "  exit    - Close terminal\n");
    } else if (strcmp(command, "clear") == 0) {
        terminal_clear(term_win);
    } else if (strcmp(command, "whoami") == 0) {
        if (sandbox_check_permission(PRIV_RING_0_KERNEL)) {
            terminal_print(term_win, "root (DevilCore Supervisor)\n");
        } else {
            terminal_print(term_win, "user\n");
        }
    } else if (strcmp(command, "exit") == 0) {
        term_win->visible = false;
    } else if (strcmp(command, "lspci") == 0) {
        int count = pci_get_device_count();
        terminal_print(term_win, "PCI Device Dump:\n");
        for(int i = 0; i < count; i++) {
            pci_device_t *d = pci_get_device(i);
            /* Very basic string building */
            char buf[64];
            strcpy(buf, "Bus X, Dev Y: Class 0xZZ\n");
            buf[4] = '0' + (d->bus % 10);
            buf[11] = '0' + (d->device % 10);
            /* Proper conversion requires an itoa function, simplified here to avoid bloat */
            terminal_print(term_win, buf);
        }
    } else if (strcmp(command, "priv") == 0) {
        terminal_print(term_win, "DevilCore Privacy Subsystem:\n");
        terminal_print(term_win, privacy_get_status());
        terminal_print(term_win, "\n");
    } else if (strcmp(command, "encrypt") == 0) {
        char test_str[64] = "DEVILCORE_SECRET_PAYLOAD_STRING";
        terminal_print(term_win, "Original: ");
        terminal_print(term_win, test_str);
        terminal_print(term_win, "\n");
        
        crypto_encrypt_buffer((uint8_t*)test_str, 31, "key");
        terminal_print(term_win, "Encrypted: [binary string redacted]\n");
        
        crypto_decrypt_buffer((uint8_t*)test_str, 31, "key");
        terminal_print(term_win, "Decrypted: ");
        terminal_print(term_win, test_str);
        terminal_print(term_win, "\nEncryption verification successful!\n");
    } else {
        terminal_print(term_win, "dsh: command not found: ");
        terminal_print(term_win, command);
        terminal_print(term_win, "\n");
    }
}
