// kernel/drivers/display/vga.c
// VGA text mode driver

#include <devil/types.h>
#include <asm/io.h>

static volatile uint16_t *vga_buffer = (volatile uint16_t*)0xB8000;
static int vga_width = 80;
static int vga_height = 25;
static int vga_x;
static int vga_y;
static uint8_t vga_color = 0x0F;

void vga_init(void) {
    vga_x = 0;
    vga_y = 0;
    vga_color = 0x0F;
    
    // Clear screen
    for (int i = 0; i < vga_width * vga_height; i++) {
        vga_buffer[i] = 0x0F00 | ' ';
    }
    
    printf("[VGA] Text mode 80x25 initialized\n");
}

void vga_set_color(uint8_t color) {
    vga_color = color;
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_x = 0;
        vga_y++;
    } else if (c == '\r') {
        vga_x = 0;
    } else if (c == '\b') {
        if (vga_x > 0) vga_x--;
    } else {
        vga_buffer[vga_y * vga_width + vga_x] = (vga_color << 8) | c;
        vga_x++;
    }
    
    if (vga_x >= vga_width) {
        vga_x = 0;
        vga_y++;
    }
    
    if (vga_y >= vga_height) {
        // Scroll
        for (int i = 0; i < vga_width * (vga_height - 1); i++) {
            vga_buffer[i] = vga_buffer[i + vga_width];
        }
        for (int i = 0; i < vga_width; i++) {
            vga_buffer[(vga_height - 1) * vga_width + i] = 0x0F00 | ' ';
        }
        vga_y = vga_height - 1;
    }
}

void vga_write(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        vga_putchar(str[i]);
    }
}