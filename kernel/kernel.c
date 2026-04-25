/*
 * DevilCore Kernel Main
 * Entry point after Multiboot loader
 */

#include "multiboot.h"
#include "gui/compositor.h"
#include "drivers/ps2.h"
#include <stdint.h>

void kernel_main(uint32_t magic, multiboot_info_t* mbi) {
    /* 1. Verify we were booted correctly */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        /* If not multiboot, we might be in trouble, but let's try to proceed if mbi looks ok */
    }

    /* 2. Check for Framebuffer info */
    if (!(mbi->flags & (1 << 12))) {
        /* No framebuffer? We'd have to fall back to VGA text mode,
           but our goal is a GUI OS. So we assume it's there. */
    }

    /* 3. Initialize GUI Compositor */
    /* Multiboot framebuffer fields are 64-bit in the spec but 32-bit used in some loaders */
    uint32_t fb_addr = (uint32_t)mbi->framebuffer_addr;
    uint32_t width   = mbi->framebuffer_width;
    uint32_t height  = mbi->framebuffer_height;
    uint32_t pitch   = mbi->framebuffer_pitch;

    compositor_init((uint32_t*)fb_addr, width, height, pitch);

    /* 4. Initialize Hardware Drivers */
    ps2_mouse_init();

    /* 5. Initialize Window Manager */
    extern void window_init();
    extern void create_window(int x, int y, int w, int h, const char *title);
    extern void draw_desktop();
    
    window_init();
    create_window(100, 100, 500, 300, "Terminal - Root");
    create_window(650, 150, 300, 400, "DevilTools");

    /* 6. Main OS Loop */
    while(1) {
        /* Update hardware state */
        ps2_poll();

        /* Render Desktop */
        draw_desktop();
        
        /* Draw cursor on top */
        compositor_draw_cursor();
        
        /* Swap buffers */
        compositor_flush();
        
        /* Small delay to prevent QEMU from eating 100% CPU in some cases */
        for(volatile int i = 0; i < 100000; i++);
    }
}