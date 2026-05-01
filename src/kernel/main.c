#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "limine.h"
#include "memory.h"
#include "framebuffer.h"
#include "wm.h"
#include "isr.h"
#include "pci.h"
#include "ata.h"
#include "rtl8139.h"
#include "io.h"
#include "shell.h"
#include "acpi.h"
#include "timer.h"
#include "filemanager.h"
#include "texteditor.h"
#include "calculator.h"
#include "sysmonitor.h"
#include "hexeditor.h"
#include "netscanner.h"
#include "logviewer.h"
#include "notes.h"
#include "settings.h"
#include "memory_compress.h"
#include "gdt.h"
#include <stdio.h>

// New Architecture Components
#include "compositor.h"
#include "slab.h"
#include "font.h"
#include "devilui.h"
#include "ipc.h"
#include "cfs_scheduler.h"

void draw_sysinfo_content(struct wm_window *win) {
    char info[128];
    char comp_info[64];
    
    // Draw background
    fb_fill_rect(win->widget.x, win->widget.y, win->widget.width, win->widget.height, 0x001a1a1a);
    
    // Logo area
    fb_fill_rect_rounded(win->widget.x + 10, win->widget.y + 10, 50, 50, 8, 0x0044ffcc);
    fb_draw_string(win->widget.x + 22, win->widget.y + 28, "DC", 0x00000000, 0);
    
    fb_draw_string(win->widget.x + 70, win->widget.y + 15, "DevilCore OS", 0x00ffffff, 0);
    fb_draw_string(win->widget.x + 70, win->widget.y + 35, "v2.0 Premium Edition", 0x00888888, 0);
    
    fb_draw_line(win->widget.x + 10, win->widget.y + 70, win->widget.x + win->widget.width - 10, win->widget.y + 70, 0x00333333);

    sprintf(info, "Architecture: x86_64 (64-bit)");
    fb_draw_string(win->widget.x + 15, win->widget.y + 85, info, 0x00cccccc, 0);

    sprintf(info, "Memory: %u MB Total RAM", (pmm_total_pages() * 4) / 1024);
    fb_draw_string(win->widget.x + 15, win->widget.y + 105, info, 0x00cccccc, 0);

    sprintf(info, "Uptime: %u sec", (uint32_t)(timer_ticks() / 100));
    fb_draw_string(win->widget.x + 15, win->widget.y + 125, info, 0x00cccccc, 0);

    memory_compression_format_stats(comp_info, sizeof(comp_info));
    fb_draw_string(win->widget.x + 15, win->widget.y + 145, comp_info, 0x00aaaaaa, 0);
    
    fb_draw_string(win->widget.x + 15, win->widget.y + 175, "The Ultimate Hacking Environment", 0x0000ff00, 0);
}

void show_sysinfo(void) {
    struct wm_window *win = wm_create_window("System Information", 300, 200, 300, 200);
    if (win) {
        win->draw_content = draw_sysinfo_content;
        wm_draw_desktop();
        fb_flip();
    }
}

void open_terminal(void) {
    struct wm_window *term = wm_create_window("DevilCore Terminal", 200, 150, 600, 400);
    if (term) {
        term->draw_content = shell_update;
        shell_init(term);
        wm_draw_desktop();
        fb_flip();
    }
}

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

static inline void cli(void) {
    __asm__ volatile ("cli");
}

static inline void sti(void) {
    __asm__ volatile ("sti");
}

__attribute__((noreturn))
static void hcf(void) {
    cli();
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static void serial_init(void) {
    const uint16_t com1 = 0x3f8;
    outb(com1 + 1, 0x00);
    outb(com1 + 3, 0x80);
    outb(com1 + 0, 0x03);
    outb(com1 + 1, 0x00);
    outb(com1 + 3, 0x03);
    outb(com1 + 2, 0xc7);
    outb(com1 + 4, 0x0b);
}

void serial_write_char(char ch) {
    const uint16_t com1 = 0x3f8;
    if (ch == '\n') {
        serial_write_char('\r');
    }
    while ((inb(com1 + 5) & 0x20) == 0) {
    }
    outb(com1, (uint8_t)ch);
}

void serial_write_string(const char *text) {
    while (*text != '\0') {
        serial_write_char(*text++);
    }
}

void serial_write_hex_u64(uint64_t value) {
    static const char hex_digits[] = "0123456789ABCDEF";
    serial_write_string("0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        serial_write_char(hex_digits[(value >> shift) & 0x0f]);
    }
}

void serial_write_dec(uint64_t value) {
    char buf[32];
    int i = 0;
    if (value == 0) {
        serial_write_char('0');
        return;
    }
    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    while (i > 0) {
        serial_write_char(buf[--i]);
    }
}

static void pci_callback(struct pci_device *pci) {
    if (pci->vendor_id == RTL8139_VENDOR_ID && pci->device_id == RTL8139_DEVICE_ID) {
        serial_write_string("Found RTL8139 Network Card\n");
        rtl8139_init(pci);
    }
}

#include "scheduler.h"

void wm_task(void) {
    uint64_t last_frame = timer_ticks();
    const uint64_t frame_interval = 2; // ~50 FPS at 100Hz timer (smoother)
    uint8_t consecutive_no_redraw = 0;
    
    for (;;) {
        struct wm_desktop *d = wm_get_desktop();
        if (d) {
            // Process events every iteration for responsiveness
            wm_refresh();
            wm_process_events();
            
            // Check if we need to redraw
            uint64_t now = timer_ticks();
            uint8_t should_redraw = d->needs_redraw;
            
            // Force occasional redraw to update cursor blink and animations
            if (now - last_frame >= 20) { // Every 200ms for cursor blink
                should_redraw = 1;
            }
            
            // Only redraw if enough time has passed (frame limiting)
            if (should_redraw && (now - last_frame >= frame_interval)) {
                wm_draw_desktop();
                d->needs_redraw = 0;
                last_frame = now;
                consecutive_no_redraw = 0;
            } else {
                consecutive_no_redraw++;
            }
            
            // If nothing to do, yield more aggressively to save CPU
            if (consecutive_no_redraw > 50) {
                for (int i = 0; i < 10; i++) scheduler_yield();
                consecutive_no_redraw = 0;
            }
        }
        scheduler_yield();
    }
}

__attribute__((noreturn))
void kernel_main(void) {
    serial_init();
    interrupt_init();
    serial_write_string("DevilCore OS v0.4 - Advanced Architecture\n");
    serial_write_string("========================================\n");
    serial_write_string("Features: Compositor + DevilUI + CFS + IPC + Slab\n");

    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        serial_write_string("ERROR: Unsupported Limine revision\n");
        hcf();
    }
    serial_write_string("[OK] Limine revision\n");

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        serial_write_string("ERROR: Framebuffer not available\n");
        hcf();
    }
    serial_write_string("[OK] Framebuffer\n");

    if (hhdm_request.response == NULL) {
        serial_write_string("ERROR: HHDM not available\n");
        hcf();
    }
    serial_write_string("[OK] HHDM\n");

    if (memmap_request.response == NULL) {
        serial_write_string("ERROR: Memory map not available\n");
        hcf();
    }
    serial_write_string("[OK] Memory map\n");

    if (kernel_address_request.response == NULL) {
        serial_write_string("ERROR: Kernel address not available\n");
        hcf();
    }
    serial_write_string("[OK] Kernel address\n");

    memory_init(memmap_request.response, hhdm_request.response, kernel_address_request.response);
    serial_write_string("[OK] Memory initialized\n");

    gdt_init();
    serial_write_string("[OK] GDT initialized\n");

    interrupt_init();
    serial_write_string("[OK] Interrupts initialized\n");

    void *rsdp_ptr = NULL;
    if (rsdp_request.response != NULL) {
        serial_write_string("ACPI: RSDP from Limine at ");
        serial_write_hex_u64(rsdp_request.response->address);
        serial_write_string("\n");
        rsdp_ptr = (void *)(rsdp_request.response->address + hhdm_offset());
    } else {
        serial_write_string("ACPI: Limine RSDP missing, using fallback...\n");
    }

    acpi_init(rsdp_ptr);

    struct madt *madt_table = acpi_find_table("APIC");
    if (madt_table) {
        serial_write_string("ACPI: MADT found!\n");
        serial_write_string("Local APIC Address: ");
        serial_write_hex_u64(madt_table->local_apic_address);
        serial_write_string("\n");
    } else {
        serial_write_string("ACPI: MADT not found.\n");
    }

    fb_init(framebuffer_request.response);
    serial_write_string("[OK] Framebuffer initialized\n");

    wm_init();
    serial_write_string("[OK] Window Manager initialized\n");

    // Add Desktop Icons for applications
    wm_add_icon("Terminal", "terminal", 40, 100, open_terminal);
    wm_add_icon("File Mgr", "filemanager", 40, 200, fm_open);
    wm_add_icon("Text Ed", "texteditor", 40, 300, te_open);
    wm_add_icon("Calc", "calculator", 40, 400, calc_open);
    
    wm_add_icon("System", "system_monitor", 140, 100, sysmonitor_open);
    wm_add_icon("Hex Edit", "document", 140, 200, hexeditor_open);
    wm_add_icon("Net Scan", "network", 140, 300, netscanner_open);
    wm_add_icon("Logs", "document", 140, 400, logviewer_open);
    
    wm_add_icon("Notes", "notes", 240, 100, notes_open);
    wm_add_icon("Settings", "settings", 240, 200, settings_open);
    wm_add_icon("Browser", "browser", 240, 300, browser_open);

    // Open initial terminal
    open_terminal();

    wm_draw_desktop();
    wm_draw_taskbar();

    pci_init();
    pci_enumerate_devices(pci_callback);
    serial_write_string("[OK] PCI initialized\n");

    ata_init();
    serial_write_string("[OK] ATA initialized\n");
    
    // Initialize memory compression
    memory_compression_init();
    serial_write_string("[OK] Memory Compression initialized\n");

    // Initialize Slab Allocator (new architecture)
    slab_init();
    serial_write_string("[OK] Slab Allocator initialized\n");

    // Initialize Font System (new architecture)
    font_system_init();
    serial_write_string("[OK] Font System initialized\n");

    // Initialize Compositor (new architecture)
    static compositor_t compositor;
    compositor_init(&compositor, (uint32_t*)framebuffer->address, 
                    framebuffer->width, framebuffer->height, framebuffer->pitch / 4);
    serial_write_string("[OK] Compositor initialized\n");

    // Initialize DevilUI (new architecture)
    devilui_init();
    serial_write_string("[OK] DevilUI initialized\n");

    // Initialize IPC (new architecture)
    ipc_init();
    serial_write_string("[OK] IPC initialized\n");

    // Initialize CFS Scheduler (new architecture)
    cfs_init();
    serial_write_string("[OK] CFS Scheduler initialized\n");

    serial_write_string("HHDM offset: ");
    serial_write_hex_u64(hhdm_offset());
    serial_write_string("\n");

    serial_write_string("Total pages: ");
    serial_write_dec(pmm_total_pages());
    serial_write_string(" (free: ");
    serial_write_dec(pmm_free_pages());
    serial_write_string(")\n");

    task_create("wm", wm_task, TASK_FLAG_KERNEL);
    serial_write_string("[OK] Window Manager task created\n");

    serial_write_string("DevilCore OS boot complete!\n");
    serial_write_string("Calling scheduler_start...\n");

    scheduler_start();
    __builtin_unreachable();

    // Should never reach here
    for (;;) {
        __asm__ volatile("hlt");
    }
}