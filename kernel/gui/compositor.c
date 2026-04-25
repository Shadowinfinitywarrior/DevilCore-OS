/*
 * DevilCore GUI Compositor
 * Double-buffered software renderer for 32bpp linear framebuffer
 */

#include "include/types.h"
#include "compositor.h"
#include "font.h"

/* ─── Framebuffer state ──────────────────────────────────────────────────── */
static uint32_t *fb_back  = NULL;   /* back-buffer (in RAM)          */
static uint32_t *fb_front = NULL;   /* front-buffer (VRAM from GRUB) */
static int       fb_width  = 0;
static int       fb_height = 0;
static int       fb_pitch  = 0;     /* bytes per scanline            */

/* ─── Mouse state ────────────────────────────────────────────────────────── */
static int mouse_x = 400;
static int mouse_y = 300;
static int mouse_left  = 0;
static int mouse_right = 0;
static int mouse_left_prev  = 0;
static int mouse_right_prev = 0;

/* ─── Keyboard state ─────────────────────────────────────────────────────── */
static uint8_t last_scancode = 0;
static char    last_ascii    = 0;

/* ─── Simple heap for back-buffer ────────────────────────────────────────── */
extern int __heap_base; /* defined in linker script */
static uint8_t *heap_ptr = NULL;

static void *kmalloc(uint32_t size) {
    if (!heap_ptr) heap_ptr = (uint8_t*)&__heap_base;
    void *p = heap_ptr;
    heap_ptr += (size + 15) & ~15u;
    return p;
}

/* ─── memcpy / memset in-kernel ──────────────────────────────────────────── */
static void *kmemset(void *dst, int val, uint32_t n) {
    uint8_t *p = dst;
    while (n--) *p++ = (uint8_t)val;
    return dst;
}
static void *kmemcpy(void *dst, const void *src, uint32_t n) {
    uint8_t *d = dst; const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

/* ─── Scancode → ASCII (US-QWERTY) ──────────────────────────────────────── */
static const char sc_map[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' '
};
static const char sc_map_shift[128] = {
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' '
};

static int shift_held = 0;

/* ── Public init ─────────────────────────────────────────────────────────── */
void compositor_init(uint32_t *framebuffer, int width, int height, int pitch_bytes) {
    fb_front  = framebuffer;
    fb_width  = width;
    fb_height = height;
    fb_pitch  = pitch_bytes;
    mouse_x   = width  / 2;
    mouse_y   = height / 2;

    /* Allocate back-buffer from kernel heap */
    fb_back = (uint32_t*)kmalloc((uint32_t)(height * pitch_bytes));
    kmemset(fb_back, 0, (uint32_t)(height * pitch_bytes));
}

/* ── Flush back→front ─────────────────────────────────────────────────────── */
void compositor_flush(void) {
    kmemcpy(fb_front, fb_back, (uint32_t)(fb_height * fb_pitch));
}

/* ── Drawing primitives (all draw into back-buffer) ─────────────────────── */
void draw_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) return;
    fb_back[y * (fb_pitch / 4) + x] = color;
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int ry = y; ry < y + h; ry++)
        for (int rx = x; rx < x + w; rx++)
            draw_pixel(rx, ry, color);
}

void draw_rect_outline(int x, int y, int w, int h, uint32_t color) {
    draw_rect(x,       y,       w, 1, color);
    draw_rect(x,       y+h-1,   w, 1, color);
    draw_rect(x,       y,       1, h, color);
    draw_rect(x+w-1,   y,       1, h, color);
}

/* Simple alpha blend: src over dst */
static uint32_t blend(uint32_t dst, uint32_t src, uint8_t alpha) {
    uint32_t rb = ((src & 0xFF00FF) * alpha + (dst & 0xFF00FF) * (255 - alpha)) >> 8;
    uint32_t g  = ((src & 0x00FF00) * alpha + (dst & 0x00FF00) * (255 - alpha)) >> 8;
    return ((rb & 0xFF00FF) | (g & 0x00FF00)) | 0xFF000000;
}

void draw_gradient_rect(int x, int y, int w, int h, uint32_t top, uint32_t bot) {
    for (int ry = 0; ry < h; ry++) {
        uint8_t a = (uint8_t)(ry * 255 / h);
        uint32_t c = blend(top, bot, a);
        for (int rx = 0; rx < w; rx++)
            draw_pixel(x + rx, y + ry, c);
    }
}

void draw_char(int x, int y, char ch, uint32_t color, uint32_t bg) {
    if ((uint8_t)ch >= 128) return;
    const uint8_t *glyph = font8x8[(uint8_t)ch];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col)))
                draw_pixel(x + col, y + row, color);
            else if (bg != 0xFF000000)  /* transparent sentinel */
                draw_pixel(x + col, y + row, bg);
        }
    }
}

void draw_string(int x, int y, const char *s, uint32_t color, uint32_t bg) {
    int cx = x;
    while (*s) {
        if (*s == '\n') { y += 10; cx = x; s++; continue; }
        draw_char(cx, y, *s, color, bg);
        cx += 9;
        s++;
    }
}

/* Scale-2× text for titles */
void draw_char2x(int x, int y, char ch, uint32_t color) {
    if ((uint8_t)ch >= 128) return;
    const uint8_t *glyph = font8x8[(uint8_t)ch];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                draw_pixel(x + col*2,   y + row*2,   color);
                draw_pixel(x + col*2+1, y + row*2,   color);
                draw_pixel(x + col*2,   y + row*2+1, color);
                draw_pixel(x + col*2+1, y + row*2+1, color);
            }
        }
    }
}

void draw_string2x(int x, int y, const char *s, uint32_t color) {
    int cx = x;
    while (*s) {
        draw_char2x(cx, y, *s, color);
        cx += 18;
        s++;
    }
}

int  compositor_get_width(void)  { return fb_width;  }
int  compositor_get_height(void) { return fb_height; }

/* ── Mouse cursor ─────────────────────────────────────────────────────────── */
static const uint8_t cursor_mask[16][12] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,2,2,1,0,0},
    {1,2,2,2,2,2,1,1,1,1,0,0},
    {1,2,2,1,2,2,1,0,0,0,0,0},
    {1,2,1,0,1,2,2,1,0,0,0,0},
    {1,1,0,0,0,1,2,2,1,0,0,0},
    {0,0,0,0,0,1,2,1,0,0,0,0},
    {0,0,0,0,0,0,1,0,0,0,0,0},
};

void compositor_draw_cursor(void) {
    for (int r = 0; r < 16; r++)
        for (int c = 0; c < 12; c++) {
            uint8_t v = cursor_mask[r][c];
            if      (v == 1) draw_pixel(mouse_x + c, mouse_y + r, 0xFFFFFFFF);
            else if (v == 2) draw_pixel(mouse_x + c, mouse_y + r, 0xFF000000);
        }
}

/* ── Input callbacks ─────────────────────────────────────────────────────── */
void compositor_move_mouse(int dx, int dy) {
    mouse_x += dx;
    mouse_y += dy;
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= fb_width)  mouse_x = fb_width  - 1;
    if (mouse_y >= fb_height) mouse_y = fb_height - 1;
}

void compositor_click_mouse(int left, int right) {
    mouse_left_prev  = mouse_left;
    mouse_right_prev = mouse_right;
    mouse_left  = left;
    mouse_right = right;
}

void compositor_keyboard_event(uint8_t scancode) {
    if (scancode == 0x2A || scancode == 0x36) { shift_held = 1; return; }
    if (scancode == 0xAA || scancode == 0xB6) { shift_held = 0; return; }
    if (scancode & 0x80) return; /* key release */
    last_scancode = scancode;
    last_ascii = shift_held ? sc_map_shift[scancode] : sc_map[scancode];
}

int  compositor_mouse_x(void)     { return mouse_x; }
int  compositor_mouse_y(void)     { return mouse_y; }
int  compositor_mouse_left(void)  { return mouse_left; }
int  compositor_mouse_right(void) { return mouse_right; }
int  compositor_mouse_clicked(void) {
    return (mouse_left && !mouse_left_prev);
}
char compositor_get_char(void) {
    char c = last_ascii;
    last_ascii = 0;
    return c;
}
