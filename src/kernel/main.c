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
#include "ahci.h"
#include "e1000.h"
#include "smp.h"
#include "logo.h"
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
    fb_fill_rect(win->widget.x, win->widget.y, win->widget.width, win->widget.height, 0x001a1a2e);
    
    // Centered Logo
    uint32_t logo_x = win->widget.x + (win->widget.width - 64) / 2;
    logo64_draw(logo_x, win->widget.y + 10, 1);
    
    fb_draw_string_centered(win->widget.x, win->widget.y + 80, win->widget.width, "DevilCore Platinum", 0x0044ffcc);
    fb_draw_string_centered(win->widget.x, win->widget.y + 100, win->widget.width, "The Ethical Hacking OS", 0x00888888);
    
    fb_draw_line(win->widget.x + 10, win->widget.y + 120, win->widget.x + win->widget.width - 10, win->widget.y + 120, 0x00333333);

    sprintf(info, "Version: 0.5.0-platinum");
    fb_draw_string(win->widget.x + 15, win->widget.y + 135, info, 0x00cccccc, 0);

    sprintf(info, "Arch: x86_64-baremetal");
    fb_draw_string(win->widget.x + 15, win->widget.y + 155, info, 0x00cccccc, 0);

    sprintf(info, "Memory: %u MB Total", (pmm_total_pages() * 4) / 1024);
    fb_draw_string(win->widget.x + 15, win->widget.y + 175, info, 0x00cccccc, 0);

    memory_compression_format_stats(comp_info, sizeof(comp_info));
    fb_draw_string(win->widget.x + 15, win->widget.y + 195, comp_info, 0x00aaaaaa, 0);
}

void show_sysinfo(void) {
    struct wm_window *win = wm_create_window("About System", 300, 200, 350, 250);
    if (win) {
        win->draw_content = draw_sysinfo_content;
    }
}

void open_terminal(void) {
    struct wm_window *term = wm_create_window("DevilCore Terminal", 200, 150, 600, 400);
    if (term) {
        term->draw_content = shell_update;
        shell_init(term);
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

__attribute__((used, section(".limine_requests")))
static volatile struct limine_smp_request smp_request = {
    .id = LIMINE_SMP_REQUEST,
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
    } else if (pci->class_code == 0x01 && pci->subclass_code == 0x06) {
        serial_write_string("Found AHCI Controller\n");
        ahci_init(pci);
    } else if (pci->vendor_id == E1000_VENDOR_ID && pci->device_id == E1000_DEVICE_ID) {
        serial_write_string("Found Intel E1000 Network Card\n");
        e1000_init(pci);
    }
}

#include "cfs_scheduler.h"
#include "syscall.h"

void wm_task(void) {
    uint64_t last_frame = timer_ticks();
    const uint64_t frame_interval = 1; // ~100 FPS at 100Hz timer (max smoothness)
    uint8_t consecutive_no_redraw = 0;
    
    serial_write_string("WM Task started\n");
    
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
            
            // If nothing to do, yield briefly to avoid hogging CPU, but keep latency low
            if (consecutive_no_redraw > 100) {
                scheduler_yield();
                consecutive_no_redraw = 0;
            }
        }
        scheduler_yield();
    }
}

__attribute__((noreturn))
void kernel_main(void) {
    serial_init();
    // interrupt_init(); // Removed early call
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
    sti(); // Enable interrupts for timer
    uint64_t boot_start_time = timer_ticks();
    serial_write_string("[OK] Interrupts initialized\n");

    syscall_init();
    serial_write_string("[OK] Syscalls initialized\n");

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

    if (smp_request.response != NULL) {
        smp_init(smp_request.response);
    } else {
        serial_write_string("SMP: Limine SMP response missing.\n");
    }

    fb_init(framebuffer_request.response);
    struct framebuffer *fb = framebuffer;
    
    // Splash screen
    if (fb) {
        fb_fill_rect(0, 0, fb->width, fb->height, 0x00000000);
        
        // Large raw logo centered (uses raw pixel data from logo_raw.h)
        uint32_t logo_x = (fb->width - 612) / 2;
        uint32_t logo_y = (fb->height - 408) / 2 - 40;
        logo_raw_draw(logo_x, logo_y);
        
        // fb_draw_string_scaled((fb->width - 9 * 8 * 3) / 2, logo_y + 408 + 20, "DevilCore", 0x00FF0000, 3);
        fb_draw_string_centered(0, fb->height - 100, fb->width, "Initializing Ethical Hacking Environment...", 0x00888888);
        fb_flip();
    }

    serial_write_string("[OK] Framebuffer initialized\n");

    wm_init();
    serial_write_string("[OK] Window Manager initialized\n");

    // Splash progress 1
    if (fb) {
        fb_fill_rect((fb->width - 300) / 2, fb->height - 60, 100, 4, 0x00FF0000);
        fb_flip();
    }

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

    // Open initial terminal and a system monitor for 'hacker' vibe
    open_terminal();
    sysmonitor_open();

    // Splash progress 2
    if (fb) {
        fb_fill_rect((fb->width - 300) / 2 + 100, fb->height - 60, 100, 4, 0x00FF0000);
        fb_flip();
    }

    // wm_draw_desktop();
    // wm_draw_taskbar();

    pci_init();
    pci_enumerate_devices(pci_callback);
    serial_write_string("[OK] PCI initialized\n");

    // Splash progress 3
    if (fb) {
        fb_fill_rect((fb->width - 300) / 2 + 200, fb->height - 60, 100, 4, 0x00FF0000);
        fb_flip();
    }

    // ata_init();
    // serial_write_string("[OK] ATA initialized\n");
    
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

    serial_write_string("DevilCore OS boot complete!\n");

    // Wait for 5 seconds total for splash screen (500 ticks at 100Hz)
    while (timer_ticks() - boot_start_time < 500) {
        __asm__ volatile("hlt");
    }

    // Initialize tasks and start scheduler after the splash screen
    task_create("wm", wm_task, TASK_FLAG_KERNEL);
    serial_write_string("[OK] Window Manager task created\n");

    // Now draw initial desktop state before starting the scheduler
    wm_draw_desktop();

    serial_write_string("Calling scheduler_start...\n");

    scheduler_start();
    __builtin_unreachable();

    // Should never reach here
    for (;;) {
        __asm__ volatile("hlt");
    }
}