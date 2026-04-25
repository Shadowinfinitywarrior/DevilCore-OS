/*
 * DevilCore OS — Diagnostic Subsystem Core
 * Dispatcher and utility functions
 */

#include "diag.h"
#include "../libc/string.h"
#include <devil/printf.h>
#include "../include/ports.h"
#include "../arch/x86_64/cpu.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../mm/heap.h"
#include "../proc/process.h"
#include "../proc/scheduler.h"
#include "../fs/vfs.h"
#include "../drivers/pci.h"
#include "../drivers/timer.h"
#include "../drivers/keyboard.h"
#include "../drivers/mouse.h"
#include "../gui/framebuffer.h"
#include "../net/net.h"
#include "../syscall/syscall.h"

/* ================================================================
 * GLOBAL STATE
 * ================================================================ */

#define DIAG_MAX_CATEGORY 12
static void (*diag_handlers[DIAG_MAX_CATEGORY + 1])(uint64_t subcmd, diag_output_t *out) = {0};

/* ================================================================
 * UTILITY FUNCTIONS
 * ================================================================ */

/*
 * diag_strcat_user - Safely append a string to user buffer
 */
int diag_strcat_user(diag_output_t *out, const char *str) {
    if (!out || !out->buffer || !str) return -1;

    size_t len = strlen(str);
    if (out->written + len >= out->size) {
        /* Not enough space */
        return -1;
    }

    /* Copy string to user buffer at current offset */
    char *dest = out->buffer + out->written;
    strcpy(dest, str);
    out->written += len;

    return 0;
}

/*
 * diag_printf_user - Formatted output to user buffer
 */
int diag_printf_user(diag_output_t *out, const char *fmt, ...) {
    if (!out || !out->buffer) return -1;

    char temp[512];
    va_list ap;
    va_start(ap, fmt);
    int len = vsprintf(temp, fmt, ap);
    va_end(ap);

    if (len < 0) return -1;

    if (out->written + (size_t)len >= out->size) {
        return -1; /* Buffer overflow */
    }

    strcpy(out->buffer + out->written, temp);
    out->written += len;

    return 0;
}

/*
 * diag_header - Print a section header
 */
void diag_header(diag_output_t *out, const char *title) {
    diag_printf_user(out, "\n");
    diag_printf_user(out, "========================================\n");
    diag_printf_user(out, "  %s\n", title);
    diag_printf_user(out, "========================================\n");
}

/*
 * diag_subheader - Print a subsection header
 */
void diag_subheader(diag_output_t *out, const char *title) {
    diag_printf_user(out, "\n--- %s ---\n", title);
}

/* ================================================================
 * CATEGORY REGISTRATION
 * ================================================================ */

void diag_register_category(uint64_t cat, void (*handler)(uint64_t, diag_output_t*)) {
    if (cat >= 1 && cat <= DIAG_MAX_CATEGORY && handler) {
        diag_handlers[cat] = handler;
    }
}

/*
 * diag_dispatch - Main syscall dispatcher
 * Called by sys_diag() in syscall_handler.c
 */
long diag_dispatch(uint64_t category, uint64_t subcmd, uint64_t buf, uint64_t size) {
    /* Validate inputs */
    if (!buf || size == 0) return -1;
    if (category < 1 || category > DIAG_MAX_CATEGORY) return -1;
    if (!diag_handlers[category]) return -1;

    /* Setup output structure */
    diag_output_t out;
    out.buffer = (char*)buf;
    out.size = size;
    out.written = 0;
    out.error = 0;

    /* Call category handler */
    diag_handlers[category](subcmd, &out);

    /* Return bytes written (or negative error) */
    return (long)out.written;
}

/* ================================================================
 * MODULE INITIALIZATION
 * ================================================================ */

/* Forward declarations of all category modules */
void diag_cpu_register(void);
void diag_memory_register(void);
void diag_process_register(void);
void diag_fs_register(void);
void diag_graphics_register(void);
void diag_io_register(void);
void diag_security_register(void);
void diag_ipc_register(void);
void diag_timing_register(void);
void diag_network_register(void);
void diag_debug_register(void);
void diag_extra_register(void);

void diag_init(void) {
    /* Register category handlers */
    diag_cpu_register();
    diag_memory_register();
    diag_process_register();
    diag_fs_register();
    diag_graphics_register();
    diag_io_register();
    diag_security_register();
    diag_ipc_register();
    diag_timing_register();
    diag_network_register();
    diag_debug_register();
    diag_extra_register();
}

/* ================================================================
 * CPU/HARDWARE MODULE (items 1-10)
 * ================================================================ */
static void diag_cpu_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_cpu_register(void) {
    diag_register_category(DIAG_CAT_CPU, diag_cpu_dispatch);
}

/* Implementation in separate file diag_cpu.c */
extern void diag_cpu_impl(uint64_t subcmd, diag_output_t *out);
static void diag_cpu_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_cpu_impl(subcmd, out);
}

/* ================================================================
 * MEMORY MODULE (items 11-20)
 * ================================================================ */
static void diag_memory_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_memory_register(void) {
    diag_register_category(DIAG_CAT_MEMORY, diag_memory_dispatch);
}

extern void diag_memory_impl(uint64_t subcmd, diag_output_t *out);
static void diag_memory_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_memory_impl(subcmd, out);
}

/* ================================================================
 * PROCESS MODULE (items 21-30)
 * ================================================================ */
static void diag_process_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_process_register(void) {
    diag_register_category(DIAG_CAT_PROCESS, diag_process_dispatch);
}

extern void diag_process_impl(uint64_t subcmd, diag_output_t *out);
static void diag_process_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_process_impl(subcmd, out);
}

/* ================================================================
 * FILESYSTEM MODULE (items 31-40)
 * ================================================================ */
static void diag_fs_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_fs_register(void) {
    diag_register_category(DIAG_CAT_FILESYSTEM, diag_fs_dispatch);
}

extern void diag_fs_impl(uint64_t subcmd, diag_output_t *out);
static void diag_fs_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_fs_impl(subcmd, out);
}

/* ================================================================
 * GRAPHICS MODULE (items 41-50)
 * ================================================================ */
static void diag_graphics_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_graphics_register(void) {
    diag_register_category(DIAG_CAT_GRAPHICS, diag_graphics_dispatch);
}

extern void diag_graphics_impl(uint64_t subcmd, diag_output_t *out);
static void diag_graphics_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_graphics_impl(subcmd, out);
}

/* ================================================================
 * I/O MODULE (items 51-60)
 * ================================================================ */
static void diag_io_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_io_register(void) {
    diag_register_category(DIAG_CAT_IO, diag_io_dispatch);
}

extern void diag_io_impl(uint64_t subcmd, diag_output_t *out);
static void diag_io_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_io_impl(subcmd, out);
}

/* ================================================================
 * SECURITY MODULE (items 61-70)
 * ================================================================ */
static void diag_security_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_security_register(void) {
    diag_register_category(DIAG_CAT_SECURITY, diag_security_dispatch);
}

extern void diag_security_impl(uint64_t subcmd, diag_output_t *out);
static void diag_security_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_security_impl(subcmd, out);
}

/* ================================================================
 * IPC MODULE (items 71-80)
 * ================================================================ */
static void diag_ipc_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_ipc_register(void) {
    diag_register_category(DIAG_CAT_IPC, diag_ipc_dispatch);
}

extern void diag_ipc_impl(uint64_t subcmd, diag_output_t *out);
static void diag_ipc_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_ipc_impl(subcmd, out);
}

/* ================================================================
 * TIMING MODULE (items 81-90)
 * ================================================================ */
static void diag_timing_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_timing_register(void) {
    diag_register_category(DIAG_CAT_TIMING, diag_timing_dispatch);
}

extern void diag_timing_impl(uint64_t subcmd, diag_output_t *out);
static void diag_timing_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_timing_impl(subcmd, out);
}

/* ================================================================
 * NETWORK MODULE (items 91-100)
 * ================================================================ */
static void diag_network_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_network_register(void) {
    diag_register_category(DIAG_CAT_NETWORK, diag_network_dispatch);
}

extern void diag_network_impl(uint64_t subcmd, diag_output_t *out);
static void diag_network_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_network_impl(subcmd, out);
}

/* ================================================================
 * DEBUG MODULE (items 101-110)
 * ================================================================ */
static void diag_debug_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_debug_register(void) {
    diag_register_category(DIAG_CAT_DEBUG, diag_debug_dispatch);
}

extern void diag_debug_impl(uint64_t subcmd, diag_output_t *out);
static void diag_debug_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_debug_impl(subcmd, out);
}

/* ================================================================
 * EXTRA MODULE (items 111-120)
 * ================================================================ */
static void diag_extra_dispatch(uint64_t subcmd, diag_output_t *out);

void diag_extra_register(void) {
    diag_register_category(DIAG_CAT_EXTRA, diag_extra_dispatch);
}

extern void diag_extra_impl(uint64_t subcmd, diag_output_t *out);
static void diag_extra_dispatch(uint64_t subcmd, diag_output_t *out) {
    diag_extra_impl(subcmd, out);
}

/* ================================================================
 * ALL-IN-ONE REPORT
 * ================================================================ */
void diag_all_report(diag_output_t *out) {
    diag_printf_user(out, "\n\n");
    diag_header(out, "DEVILCORE OS COMPREHENSIVE DIAGNOSTIC REPORT");
    diag_printf_user(out, "Generated by DevilCore Diagnostic Subsystem v1.0\n");
    diag_printf_user(out, "Total Diagnostic Items: 120 across 12 categories\n");
    diag_printf_user(out, "========================================\n\n");

    for (int cat = 1; cat <= DIAG_MAX_CATEGORY; cat++) {
        if (diag_handlers[cat]) {
            /* Call handler with special subcmd 0 = "all" */
            diag_handlers[cat](0, out);
        }
    }

    diag_header(out, "END OF REPORT");
    diag_printf_user(out, "\n");
}
