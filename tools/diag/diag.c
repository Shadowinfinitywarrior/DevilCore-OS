/*
 * DevilCore OS — Diagnostic Tool (userspace)
 *
 * This program invokes the SYS_DIAG syscall to retrieve system diagnostics.
 * Compile with: make
 *
 * Usage: diag [category [subcommand]]
 *   Without args: run all diagnostic checks (category=0)
 *   With category: run specific category (e.g., 1 for CPU)
 *   With subcmd: specific item within category
 *
 * Example: diag 1 5   => Show GDT entries
 *          diag      => Full report
 */

#include <stdint.h>
#include <stddef.h>

/* Syscall number */
#define SYS_DIAG 300

/* Syscall prototypes - same as kernel diag.h */
typedef enum {
    DIAG_CAT_CPU = 1,
    DIAG_CAT_MEMORY,
    DIAG_CAT_PROCESS,
    DIAG_CAT_FILESYSTEM,
    DIAG_CAT_GRAPHICS,
    DIAG_CAT_IO,
    DIAG_CAT_SECURITY,
    DIAG_CAT_IPC,
    DIAG_CAT_TIMING,
    DIAG_CAT_NETWORK,
    DIAG_CAT_DEBUG,
    DIAG_CAT_EXTRA,
    DIAG_CAT_ALL
} diag_category_t;

/* Syscall wrapper - x86_64 Syscall ABI */
static inline long sys_diag(uint64_t cat, uint64_t sub, void *buf, uint64_t sz) {
    long ret;
    register uint64_t r10 __asm__("r10") = sz;
    __asm__ volatile ("syscall"
        : "=a"(ret)
        : "a"(SYS_DIAG), "D"(cat), "S"(sub), "d"(buf), "r"(r10)
        : "rcx", "r11", "memory");
    return ret;
}

/* Simple main */
int main(int argc, char **argv) {
    uint64_t category = DIAG_CAT_ALL;   /* default: all */
    uint64_t subcmd   = 0;

    if (argc >= 2) {
        category = (uint64_t)atoi(argv[1]);
    }
    if (argc >= 3) {
        subcmd = (uint64_t)atoi(argv[2]);
    }

    /* Allocate buffer for response */
    char buffer[65536];
    long ret = sys_diag(category, subcmd, buffer, sizeof(buffer)-1);
    if (ret < 0) {
        /* Error */
        const char *err = "Diagnostic syscall failed (kernel support missing?)\n";
        /* We'll just print via host if possible */
        write(1, err, 50);
        return 1;
    }

    buffer[ret] = '\0';
    write(1, buffer, ret);
    return 0;
}
