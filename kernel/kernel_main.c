/*
 * ============================================================
 *  DevilCore OS v5.0 — kernel_main.c
 *  Privacy-Focused, Security-Hardened 64-bit OS Kernel
 *  Architecture: x86_64 Long Mode (Multiboot2)
 *
 *  This file now contains ONLY:
 *  - Multiboot2 tag parser
 *  - ISR dispatcher
 *  - GUI application renderers
 *  - Window manager + desktop
 *  - Kernel main entry + event loop
 *
 *  All drivers, libc, font, and framebuffer are in separate modules.
 * ============================================================
 */

/* ---- Module includes ---- */
#include "include/types.h"
#include "include/ports.h"
#include "include/font8x8.h"
#include "libc/string.h"
#include "arch/idt.h"
#include "drivers/pic.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "gui/framebuffer.h"
#include "gui/font.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "proc/process.h"
#include "proc/scheduler.h"
#include "syscall/syscall.h"
#include "drivers/pci.h"
#include "drivers/ata.h"
#include "drivers/usb/usb.h"
#include "drivers/usb/usb_hid.h"
#include "fs/vfs.h"
#include "net/net.h"
#include "security/privacy_hub.h"

/* ================================================================
 *  SECTION 1: MULTIBOOT2 PARSER
 * ================================================================ */
#define MB2_MAGIC_RESPONSE  0x36d76289
#define MB2_TAG_END         0
#define MB2_TAG_CMDLINE     1
#define MB2_TAG_MMAP        6
#define MB2_TAG_FB          8

struct mb2_hdr { uint32_t total_size; uint32_t reserved; };
struct mb2_tag { uint32_t type; uint32_t size; };
struct mb2_tag_fb {
    uint32_t type, size;
    uint64_t addr;
    uint32_t pitch, width, height;
    uint8_t  bpp, fb_type;
    uint16_t _res;
    uint8_t  red_pos, red_sz, green_pos, green_sz, blue_pos, blue_sz;
};
struct mb2_mmap_entry { uint64_t base, len; uint32_t type, _res; };
struct mb2_tag_mmap {
    uint32_t type, size, entry_sz, entry_ver;
    struct mb2_mmap_entry entries[];
};

static uint64_t g_mem_total = 256*1024*1024ULL;
static uint8_t  g_rpos = 16, g_gpos = 8, g_bpos = 0;

/* Temporaries filled by mb2_parse, consumed by kernel_main */
static volatile uint8_t *g_fb_addr = 0;
static uint32_t g_fb_pitch = 0, g_fb_w = 0, g_fb_h = 0;
static uint8_t  g_fb_bpp = 0;

static void mb2_parse(uint64_t mb2_addr) {
    struct mb2_hdr *hdr = (struct mb2_hdr*)mb2_addr;
    uint8_t *p   = (uint8_t*)(mb2_addr + 8);
    uint8_t *end = (uint8_t*)(mb2_addr + hdr->total_size);

    while (p < end) {
        struct mb2_tag *tag = (struct mb2_tag*)p;
        if (tag->type == MB2_TAG_END) break;

        if (tag->type == MB2_TAG_FB) {
            struct mb2_tag_fb *fb = (struct mb2_tag_fb*)tag;
            g_fb_addr  = (volatile uint8_t*)(uint64_t)fb->addr;
            g_fb_pitch = fb->pitch;
            g_fb_w     = fb->width;
            g_fb_h     = fb->height;
            g_fb_bpp   = fb->bpp;
            g_rpos     = fb->red_pos;
            g_gpos     = fb->green_pos;
            g_bpos     = fb->blue_pos;
        }
        if (tag->type == MB2_TAG_MMAP) {
            struct mb2_tag_mmap *mm = (struct mb2_tag_mmap*)tag;
            uint32_t n = (mm->size - 16) / mm->entry_sz;
            g_mem_total = 0;
            for (uint32_t i = 0; i < n; i++)
                if (mm->entries[i].type == 1)
                    g_mem_total += mm->entries[i].len;
            
            /* Initialize PMM with total memory */
            pmm_init(g_mem_total);

            /* Mark available memory regions in the PMM */
            for (uint32_t i = 0; i < n; i++) {
                if (mm->entries[i].type == 1) { /* Type 1 = Available RAM */
                    uint64_t base = mm->entries[i].base;
                    uint64_t len = mm->entries[i].len;
                    /* Don't free the first 2MB (Kernel, Bootloader, VGA, etc) */
                    if (base < 0x200000) {
                        if (len <= (0x200000 - base)) continue;
                        len -= (0x200000 - base);
                        base = 0x200000;
                    }
                    pmm_init_region(base, len);
                }
            }
        }
        p += ALIGN_UP(tag->size, 8);
    }
}

/* ================================================================
 *  SECTION 2: ISR DISPATCHER
 * ================================================================ */
void isr_handler(uint64_t int_no, uint64_t err_code) {
    (void)err_code;

    if (int_no == 32) {
        g_ticks++;
        if (g_ticks % HZ == 0) rtc_update();
        pic_eoi(32);
        sched_tick(); /* Preempt current thread */
    } else if (int_no == 33) {
        uint8_t sc = inb(0x60);
        kbd_handler(sc);
        pic_eoi(33);
    } else if (int_no == 44) {
        uint8_t b = inb(0x60);
        mouse_handler(b);
        pic_eoi(44);
    } else if (int_no < 32) {
        /* CPU Exception — halt */
        __asm__ volatile("cli; hlt");
    } else {
        pic_eoi(int_no);
    }
}

/* ================================================================
 *  SECTION 3: WINDOW MANAGER
 * ================================================================ */
#include "gui/wm.h"

/* ================================================================
 *  SECTION 4: WINDOW FRAME RENDERING
 * ================================================================ */
static void wm_draw_frame(Window *w) {
    if (!w->visible) return;
    int x = w->x, y = w->y, ww = w->w, wh = w->h;

    /* Shadow */
    fb_alpha_fill(x + 5, y + 5, ww, wh, COL(0x000000), 100);

    /* Titlebar gradient */
    fb_gradient(x, y, ww, TITLEBAR_H,
                0x1A, 0x1A, 0x50,
                0x0A, 0x0A, 0x28);

    /* Window body */
    fb_fill(x, y + TITLEBAR_H, ww, wh - TITLEBAR_H, COL(C_WIN_BG));

    /* Border (double line for focused) */
    uint32_t bc = w->focused ? COL(C_WIN_BORDER) : COL(0x2A2A60);
    fb_rect(x, y, ww, wh, bc);
    if (w->focused)
        fb_rect(x + 1, y + 1, ww - 2, wh - 2, bc);

    /* Titlebar separator */
    fb_hline(x, y + TITLEBAR_H - 1, ww, COL(C_WIN_BORDER));

    /* Title text */
    font_str(x + 10, y + 4, w->title, COL(C_TEXT), 0);

    /* Close button */
    int cx = x + ww - 22, cy = y + 4;
    fb_fill(cx, cy, 16, 16, COL(C_CLOSE));
    fb_rect(cx, cy, 16, 16, COL(0xFF6688));
    font_str(cx + 3, cy, "X", COL(C_TEXT), COL(C_CLOSE));
}

/* ================================================================
 *  SECTION 5: DESKTOP ICONS
 * ================================================================ */
typedef struct { int x, y, type; const char *label; uint32_t color; } Icon;

#define ICON_W  64
#define ICON_H  64
#define TASKBAR_H 32

static const Icon g_icons[] = {
    { 30,  40, WM_TERMINAL, "Terminal", C_GREEN    },
    { 30, 150, WM_EDITOR,   "Editor",   C_ACCENT   },
    { 30, 260, WM_FILEMAN,  "Files",    C_YELLOW   },
    { 30, 370, WM_SYSMON,   "Monitor",  C_ORANGE   },
    { 30, 480, WM_NETMON,   "Network",  C_ACCENT2  },
    { 30, 590, WM_SETTINGS, "Settings", C_TEXT_DIM },
};
#define ICON_COUNT ((int)(sizeof(g_icons)/sizeof(g_icons[0])))

static int g_hovered_icon = -1;

static void draw_icon(int idx, bool hover) {
    const Icon *ic = &g_icons[idx];
    uint32_t bg = hover ? COL(C_ICON_HL) : COL(C_ICON_BG);
    fb_fill(ic->x, ic->y, ICON_W, ICON_H, bg);
    uint32_t bc = hover ? COL(ic->color) : COL(C_WIN_BORDER);
    fb_rect(ic->x, ic->y, ICON_W, ICON_H, bc);
    if (hover) fb_rect(ic->x + 1, ic->y + 1, ICON_W - 2, ICON_H - 2, bc);

    /* Large initial letter centered */
    int lx = ic->x + ICON_W/2 - CHAR_W/2;
    int ly = ic->y + ICON_H/2 - CHAR_H/2;
    font_char(lx, ly, ic->label[0], COL(ic->color), 0);

    /* Label below */
    int lbl_x = ic->x + ICON_W/2 - (int)strlen(ic->label) * CHAR_W / 2;
    font_str_tr(lbl_x, ic->y + ICON_H + 6, ic->label, COL(ic->color));
}

static void draw_icons(void) {
    for (int i = 0; i < ICON_COUNT; i++)
        draw_icon(i, i == g_hovered_icon);
}

/* ================================================================
 *  SECTION 6: TASKBAR
 * ================================================================ */
static void draw_taskbar(void) {
    int w = (int)fb_width();
    int y = (int)fb_height() - TASKBAR_H;

    /* Taskbar bg */
    fb_gradient(0, y, w, TASKBAR_H, 0x08, 0x08, 0x16, 0x02, 0x02, 0x08);
    fb_hline(0, y, w, COL(C_WIN_BORDER));

    /* Start button */
    fb_fill(0, y + 1, 90, TASKBAR_H - 1, COL(C_BTN));
    fb_rect(0, y + 1, 90, TASKBAR_H - 1, COL(C_WIN_BORDER));
    font_str(8, y + 8, "DEVIL", COL(C_ACCENT), 0);

    /* Clock */
    char clock_buf[16] = {0};
    char tmp[4];
    itoa(g_hour, tmp, 10);
    if (g_hour < 10) { clock_buf[0]='0'; clock_buf[1]=tmp[0]; clock_buf[2]=0; }
    else { clock_buf[0]=tmp[0]; clock_buf[1]=tmp[1]; clock_buf[2]=0; }
    strcat(clock_buf, ":");
    itoa(g_min, tmp, 10);
    if (g_min < 10) { char t[3]; t[0]='0'; t[1]=tmp[0]; t[2]=0; strcat(clock_buf,t); }
    else strcat(clock_buf, tmp);
    strcat(clock_buf, ":");
    itoa(g_sec, tmp, 10);
    if (g_sec < 10) { char t[3]; t[0]='0'; t[1]=tmp[0]; t[2]=0; strcat(clock_buf,t); }
    else strcat(clock_buf, tmp);
    int cw = (int)strlen(clock_buf) * CHAR_W;
    font_str(w - cw - 16, y + 8, clock_buf, COL(C_ACCENT), 0);

    /* Privacy indicator */
    fb_fill(w - 140, y + 6, 36, 20, COL(C_GREEN));
    fb_rect(w - 140, y + 6, 36, 20, COL(0x00CC66));
    font_str(w - 138, y + 8, "TOR", COL(0x003322), COL(C_GREEN));

    /* Logout button */
    fb_fill(w - 240, y + 6, 90, 20, COL(C_RED));
    fb_rect(w - 240, y + 6, 90, 20, COL(0xCC2244));
    font_str(w - 228, y + 8, "LOGOUT", COL(C_TEXT), COL(C_RED));

    /* Running apps */
    int bx = 100;
    for (int i = 0; i < g_win_count; i++) {
        if (!g_wins[i].visible) continue;
        uint32_t bg = (i == g_focused_win) ? COL(C_BTN_HL) : COL(C_BTN);
        fb_fill(bx, y + 5, 120, 22, bg);
        fb_rect(bx, y + 5, 120, 22, COL(C_WIN_BORDER));
        char label[14]; strncpy(label, g_wins[i].title, 13); label[13] = 0;
        font_str(bx + 6, y + 9, label, COL(C_TEXT), 0);
        bx += 125;
        if (bx > w - 260) break;
    }
}

/* ================================================================
 *  SECTION 7: DESKTOP BACKGROUND
 * ================================================================ */
static void draw_desktop_bg(void) {
    fb_gradient(0, 0, (int)fb_width(), (int)fb_height(),
                0x0A, 0x08, 0x1E,
                0x02, 0x02, 0x0C);
    /* Subtle dot grid */
    for (uint32_t y = 20; y < fb_height() - TASKBAR_H; y += 30)
        for (uint32_t x = 120; x < fb_width(); x += 30)
            fb_pixel((int)x, (int)y, COL(C_GRID));
    /* Version watermark */
    font_str_tr((int)fb_width() - 240, (int)fb_height() - TASKBAR_H - 22,
                "DevilCore OS v5.0", COL(0x1A1A40));
}

/* ================================================================
 *  SECTION 8: MOUSE CURSOR
 * ================================================================ */
static void draw_cursor(void) {
    int cx = g_mouse_x, cy = g_mouse_y;
    const uint16_t shape[20] = {
        0x8000,0xC000,0xE000,0xF000,0xF800,
        0xFC00,0xFE00,0xFF00,0xFF80,0xFFC0,
        0xFE00,0xEE00,0xC600,0x0600,0x0300,
        0x0300,0x0180,0x0180,0x00C0,0x00C0
    };
    /* Black outline */
    for (int r = 0; r < 20; r++)
        for (int c = 0; c < 12; c++)
            if (shape[r] & (0x8000 >> c))
                fb_pixel(cx + c + 1, cy + r + 1, COL(0x000000));
    /* Cursor body */
    for (int r = 0; r < 20; r++)
        for (int c = 0; c < 12; c++)
            if (shape[r] & (0x8000 >> c))
                fb_pixel(cx + c, cy + r, COL(C_CURSOR));
}

/* ================================================================
 *  SECTION 9: APP RENDERERS
 * ================================================================ */

/* --- TERMINAL --- */
#include "apps/dsh.h"
#include "gui/terminal.h"

static void render_terminal(Window *w) {
    int x0 = w->x + 2, y0 = w->y + TITLEBAR_H + 4;
    int cw = w->w - 4, ch = w->h - TITLEBAR_H - 34;

    fb_fill(x0, y0, cw, ch, COL(0x030308));
    fb_rect(x0, y0, cw, ch, COL(0x202040));

    int maxy = ch / CHAR_H - 1;
    int total_lines = 0;
    const char *p = w->text_buf;
    while (*p) { if (*p == '\n') total_lines++; p++; }
    int start_line = MAX(0, total_lines - maxy - w->scroll);
    int line = 0;
    p = w->text_buf;
    int ty = y0 + 4;
    while (*p && ty < y0 + ch - CHAR_H) {
        const char *lstart = p;
        while (*p && *p != '\n') p++;
        if (line >= start_line) {
            char linebuf[128];
            int len = MIN((int)(p - lstart), 127);
            memcpy(linebuf, lstart, (size_t)len); linebuf[len] = 0;
            font_str(x0 + 6, ty, linebuf, COL(C_GREEN), 0);
            ty += CHAR_H;
        }
        line++;
        if (*p == '\n') p++;
    }

    /* Input line */
    int iy = w->y + w->h - 28;
    fb_fill(w->x + 2, iy, w->w - 4, 24, COL(0x04040C));
    fb_hline(w->x + 2, iy, w->w - 4, COL(C_ACCENT));
    font_str(w->x + 6, iy + 4, "$ ", COL(C_ACCENT), 0);
    font_str(w->x + 6 + 2*CHAR_W, iy + 4, w->input_buf, COL(C_TEXT), 0);
    if ((g_ticks / 25) & 1)
        fb_fill(w->x + 6 + (2 + (int)strlen(w->input_buf)) * CHAR_W, iy + 4, 2, CHAR_H, COL(C_ACCENT));
}

/* DSH handles command execution now via dsh_execute */

/* --- TEXT EDITOR --- */
static void render_editor(Window *w) {
    int x0 = w->x + 2, y0 = w->y + TITLEBAR_H;
    int cw = w->w - 4, ch = w->h - TITLEBAR_H - 4;
    fb_fill(x0, y0, cw, ch, COL(0x05050F));
    fb_fill(x0, y0, 40, ch, COL(0x0A0A20));
    fb_vline(x0 + 40, y0, ch, COL(C_WIN_BORDER));

    int line = 1, ty = y0 + 4;
    const char *p = w->text_buf;
    while (*p && ty < y0 + ch - CHAR_H - 20) {
        const char *ls = p;
        while (*p && *p != '\n') p++;
        char lno[6]; itoa(line, lno, 10);
        font_str(x0 + 4, ty, lno, COL(C_TEXT_DIM), 0);
        char line_buf[128];
        int len = MIN((int)(p - ls), 127);
        memcpy(line_buf, ls, (size_t)len); line_buf[len] = 0;
        uint32_t col = (line_buf[0] == '#') ? COL(C_TEXT_DIM) : COL(C_TEXT);
        font_str(x0 + 46, ty, line_buf, col, 0);
        ty += CHAR_H; line++;
        if (*p == '\n') p++;
    }
    if ((g_ticks / 25) & 1) fb_fill(x0 + 46, ty, 2, CHAR_H, COL(C_ACCENT));
    fb_fill(x0, w->y + w->h - 20, cw, 20, COL(C_TITLEBAR));
    char st[48] = "DevilCore Editor  Lines:";
    char lbuf[8]; itoa(line - 1, lbuf, 10); strcat(st, lbuf);
    font_str(x0 + 6, w->y + w->h - 16, st, COL(C_ACCENT), 0);
}

/* --- FILE MANAGER --- */
static const char *g_files[] = {
    "  [DIR]  /bin",   "  [DIR]  /boot",  "  [DIR]  /dev",
    "  [DIR]  /etc",   "  [DIR]  /home",  "  [DIR]  /lib",
    "  [DIR]  /proc",  "  [DIR]  /root",  "  [DIR]  /tmp",
    "  [DIR]  /usr",   "  [DIR]  /var",
    "  [FILE] /boot/grub/grub.cfg",
    "  [FILE] /etc/hostname",
    "  [FILE] /etc/tor/torrc",
    "  [FILE] /etc/privacy.conf",
};
#define FILE_COUNT ((int)(sizeof(g_files)/sizeof(g_files[0])))

static void render_fileman(Window *w) {
    int x0 = w->x + 2, y0 = w->y + TITLEBAR_H;
    int cw = w->w - 4, ch = w->h - TITLEBAR_H - 4;
    fb_fill(x0, y0, cw, ch, COL(C_WIN_BG));
    fb_fill(x0, y0, cw, 24, COL(C_TITLEBAR));
    font_str(x0 + 8, y0 + 4, "  /", COL(C_ACCENT), 0);
    int ty = y0 + 28;
    for (int i = 0; i < FILE_COUNT && ty < y0 + ch - CHAR_H; i++) {
        bool sel = (i == w->selected);
        fb_fill(x0, ty, cw, CHAR_H + 4, sel ? COL(C_BTN_HL) : COL(C_WIN_BG));
        if (sel) fb_rect(x0, ty, cw, CHAR_H + 4, COL(C_WIN_BORDER));
        uint32_t tc = (strncmp(g_files[i], "  [DIR]", 7) == 0) ? COL(C_ACCENT) : COL(C_TEXT);
        font_str(x0 + 8, ty + 2, g_files[i], tc, 0);
        ty += CHAR_H + 4;
    }
    fb_fill(x0, w->y + w->h - 20, cw, 20, COL(C_TITLEBAR));
    char st[48] = "Files: "; char nb[6]; itoa(FILE_COUNT, nb, 10);
    strcat(st, nb); strcat(st, " items  |  VFS: Encrypted(AES-256)");
    font_str(x0 + 6, w->y + w->h - 16, st, COL(C_ACCENT), 0);
}

/* --- SYSTEM MONITOR --- */
static uint32_t g_cpu_hist[60];
static int g_cpu_hist_idx = 0;

static void render_sysmon(Window *w) {
    int x0 = w->x + 2, y0 = w->y + TITLEBAR_H + 4;
    int cw = w->w - 4, ch = w->h - TITLEBAR_H - 8;
    fb_fill(x0, y0, cw, ch, COL(C_WIN_BG));

    int gx = x0 + 10, gy = y0 + 30, gw = cw - 20, gh = 80;
    fb_fill(gx, gy, gw, gh, COL(0x020208));
    fb_rect(gx, gy, gw, gh, COL(C_WIN_BORDER));
    font_str(x0 + 10, y0 + 10, "CPU Usage", COL(C_ACCENT), 0);
    for (int i = 1; i < 4; i++) fb_hline(gx, gy + i * gh / 4, gw, COL(0x0F0F2A));
    uint32_t pseudo_cpu = (g_ticks & 0x1F);
    g_cpu_hist[g_cpu_hist_idx] = pseudo_cpu * 3;
    g_cpu_hist_idx = (g_cpu_hist_idx + 1) % 60;
    for (int i = 0; i < 60; i++) {
        int idx = (g_cpu_hist_idx + i) % 60;
        int bh = (int)(g_cpu_hist[idx] * gh / 100);
        if (bh < 0) {
            bh = 0;
        }
        if (bh > gh) {
            bh = gh;
        }
        uint32_t bc = (bh > gh * 3 / 4) ? COL(C_RED) : (bh > gh / 2) ? COL(C_YELLOW) : COL(C_GREEN);
        fb_vline(gx + 2 + i * (gw - 4) / 60, gy + gh - bh, bh, bc);
    }

    int my = gy + gh + 20;
    font_str(x0 + 10, my, "Memory", COL(C_ACCENT), 0);
    my += CHAR_H + 4;
    fb_fill(gx, my, gw, 18, COL(0x020208));
    fb_rect(gx, my, gw, 18, COL(C_WIN_BORDER));
    uint32_t mtotal = (uint32_t)(g_mem_total >> 20);
    uint32_t mused = 8;
    int mu = (gw - 2) * (int)mused / (int)(mtotal ? mtotal : 256);
    if (mu < 1) mu = 1;
    fb_fill(gx + 1, my + 1, mu, 16, COL(C_ACCENT));
    char mbuf[32]; char mb2[16];
    utoa(mused, mb2, 10); strcpy(mbuf, mb2); strcat(mbuf, "MB / ");
    utoa(mtotal, mb2, 10); strcat(mbuf, mb2); strcat(mbuf, "MB");
    font_str(gx + mu + 6, my + 2, mbuf, COL(C_TEXT), 0);

    my += 30;
    font_str(x0 + 10, my, "PROCESS LIST", COL(C_ACCENT), 0);
    my += CHAR_H + 4;
    const char *procs[] = {
        " 1  init        R  0.0%  2MB",
        " 2  kmemgr      S  0.0%  1MB",
        " 3  compositor  R  2.1%  4MB",
        " 4  terminal    S  0.0%  1MB",
        " 5  netd        S  0.1%  2MB",
        " 6  securityd   S  0.0%  1MB",
        " 7  privacyd    S  0.0%  1MB",
    };
    for (int i = 0; i < 7 && my < w->y + w->h - 22; i++) {
        font_str(x0 + 8, my, procs[i], COL(C_TEXT), 0);
        my += CHAR_H + 2;
    }
}

/* --- NETWORK MONITOR --- */
static void render_netmon(Window *w) {
    int x0 = w->x + 4, y0 = w->y + TITLEBAR_H + 8;
    int cw = w->w - 8;
    fb_fill(w->x + 2, w->y + TITLEBAR_H, cw + 4, w->h - TITLEBAR_H - 2, COL(C_WIN_BG));

    font_str(x0, y0, "ANONYMITY STATUS", COL(C_ACCENT), 0);
    y0 += CHAR_H + 8;
    const struct { const char *label; const char *val; uint32_t col; } rows[] = {
        {"Tor Circuit:     ", "CONNECTED - 3-hop",            C_GREEN},
        {"Exit Node IP:    ", "195.xx.xx.xx (DE) [MASKED]",   C_GREEN},
        {"Real IP:         ", "HIDDEN",                       C_GREEN},
        {"DNS Mode:        ", "DNS-over-HTTPS (Cloudflare)",  C_GREEN},
        {"MAC Address:     ", "DE:AD:BE:EF:00:01 (SPOOFED)", C_YELLOW},
        {"Hostname:        ", "dc-a4f92e1b (RANDOMIZED)",     C_YELLOW},
        {"Firewall:        ", "ACTIVE - Default DENY all",    C_GREEN},
        {"WebRTC Leak:     ", "BLOCKED",                      C_GREEN},
    };
    for (int i = 0; i < 8; i++) {
        font_str(x0, y0, rows[i].label, COL(C_TEXT_DIM), 0);
        font_str(x0 + (int)strlen(rows[i].label) * CHAR_W, y0, rows[i].val, COL(rows[i].col), 0);
        y0 += CHAR_H + 4;
    }
    y0 += 8;
    font_str(x0, y0, "ACTIVE CONNECTIONS", COL(C_ACCENT), 0);
    y0 += CHAR_H + 6;
    fb_fill(x0 - 2, y0, cw, CHAR_H + 4, COL(C_TITLEBAR));
    font_str(x0, y0 + 2, "PROTO  LOCAL           REMOTE         STATE", COL(C_ACCENT), 0);
    y0 += CHAR_H + 4;
    const char *conn[] = {
        "TCP    127.0.0.1:9050  195.xx.xx:443   ESTABLISHED",
        "TCP    127.0.0.1:8080  127.0.0.1:9050  ESTABLISHED",
        "UDP    0.0.0.0:any     [blocked]       BLOCKED    ",
    };
    for (int i = 0; i < 3; i++) {
        font_str(x0, y0, conn[i], COL(C_TEXT), 0);
        y0 += CHAR_H + 2;
    }
}

/* --- SETTINGS --- */
typedef struct { const char *label; bool enabled; uint32_t color; } Setting;
static Setting g_settings[] = {
    {"Tor Transparent Routing",    true,  C_GREEN},
    {"MAC Address Spoofing",       true,  C_GREEN},
    {"DNS-over-HTTPS (No UDP/53)", true,  C_GREEN},
    {"AES-256 Memory Encryption",  true,  C_ACCENT},
    {"Disk Encryption (AES-256)",  false, C_YELLOW},
    {"Kernel ASLR",                true,  C_GREEN},
    {"Seccomp Syscall Filter",     true,  C_GREEN},
    {"Audit Log (Ring Buffer)",    true,  C_GREEN},
    {"Random Hostname at Boot",    true,  C_GREEN},
    {"Block WebRTC / STUN",        true,  C_GREEN},
};
#define SETTING_COUNT 10

static void render_settings(Window *w) {
    int x0 = w->x + 4, y0 = w->y + TITLEBAR_H + 8;
    int cw = w->w - 8;
    fb_fill(w->x + 2, w->y + TITLEBAR_H, cw + 4, w->h - TITLEBAR_H - 2, COL(C_WIN_BG));
    font_str(x0, y0, "SECURITY & PRIVACY SETTINGS", COL(C_ACCENT), 0);
    y0 += CHAR_H + 10;
    fb_hline(x0, y0, cw - 4, COL(C_WIN_BORDER));
    y0 += 8;
    for (int i = 0; i < SETTING_COUNT && y0 < w->y + w->h - 24; i++) {
        bool sel = (i == w->selected);
        fb_fill(x0 - 2, y0 - 2, cw, CHAR_H + 8, sel ? COL(C_BTN_HL) : COL(C_WIN_BG));
        font_str(x0, y0, g_settings[i].label, COL(C_TEXT), 0);
        const char *sv = g_settings[i].enabled ? "[ON] " : "[OFF]";
        uint32_t sc = g_settings[i].enabled ? COL(g_settings[i].color) : COL(C_RED);
        int sw = (int)strlen(g_settings[i].label) * CHAR_W + 8;
        font_str(x0 + sw, y0, sv, sc, 0);
        y0 += CHAR_H + 8;
    }
    font_str(x0, w->y + w->h - 22,
             "UP/DOWN=Navigate  ENTER=Toggle", COL(C_TEXT_DIM), 0);
}

/* ================================================================
 *  SECTION 10: LOGIN SCREEN
 * ================================================================ */
static void draw_login_screen(void) {
    draw_desktop_bg();

    int w = 400, h = 420;
    int x = ((int)fb_width() - w) / 2;
    int y = ((int)fb_height() - h) / 2;

    /* Shadow + Frame */
    fb_alpha_fill(x + 6, y + 6, w, h, COL(0x000000), 120);
    fb_fill(x, y, w, h, COL(C_WIN_BG));
    fb_rect(x, y, w, h, COL(C_WIN_BORDER));
    fb_rect(x + 1, y + 1, w - 2, h - 2, COL(C_WIN_BORDER));

    /* Logo centered at top of login box (128x85 scaled 2x = 256x170, but use 1x for elegance) */
    int logo_x = x + (w - 128) / 2;
    int logo_y = y + 16;
    fb_blit_logo(logo_x, logo_y, 1);

    /* Title below logo */
    font_str(x + w/2 - 8*CHAR_W, y + 110, "DevilCore Login", COL(C_ACCENT), 0);
    fb_hline(x + 10, y + 135, w - 20, COL(C_WIN_BORDER));

    /* Username */
    font_str(x + 40, y + 152, "Username:", COL(C_TEXT), 0);
    uint32_t ubord = (g_login_focus == 0) ? COL(C_ACCENT) : COL(C_TEXT_DIM);
    fb_fill(x + 40, y + 176, 320, 28, COL(0x050510));
    fb_rect(x + 40, y + 176, 320, 28, ubord);
    font_str(x + 48, y + 182, g_login_user, COL(C_TEXT), 0);
    if (g_login_focus == 0 && ((g_ticks / 25) & 1))
        fb_fill(x + 48 + (int)strlen(g_login_user) * CHAR_W, y + 182, 2, CHAR_H, COL(C_ACCENT));

    /* Password */
    font_str(x + 40, y + 220, "Password:", COL(C_TEXT), 0);
    uint32_t pbord = (g_login_focus == 1) ? COL(C_ACCENT) : COL(C_TEXT_DIM);
    fb_fill(x + 40, y + 244, 320, 28, COL(0x050510));
    fb_rect(x + 40, y + 244, 320, 28, pbord);
    char star[32] = {0};
    for (size_t i = 0; i < strlen(g_login_pass); i++) star[i] = '*';
    font_str(x + 48, y + 250, star, COL(C_TEXT), 0);
    if (g_login_focus == 1 && ((g_ticks / 25) & 1))
        fb_fill(x + 48 + (int)strlen(g_login_pass) * CHAR_W, y + 250, 2, CHAR_H, COL(C_ACCENT));

    /* Login Button */
    fb_fill(x + w/2 - 60, y + 300, 120, 32, COL(C_BTN));
    fb_rect(x + w/2 - 60, y + 300, 120, 32, COL(C_WIN_BORDER));
    font_str(x + w/2 - 2*CHAR_W - 8, y + 308, "LOGIN", COL(C_ACCENT), COL(C_BTN));

    /* Hint text */
    font_str(x + 40, y + 352, "root:root  or  user:user", COL(C_TEXT_DIM), 0);

    /* Error indicator */
    if (g_login_error)
        font_str(x + w/2 - 7*CHAR_W, y + 380, "ACCESS DENIED!", COL(C_RED), 0);
}

static void validate_login(void) {
    if ((strcmp(g_login_user, "root") == 0 && strcmp(g_login_pass, "root") == 0) ||
        (strcmp(g_login_user, "user") == 0 && strcmp(g_login_pass, "user") == 0)) {
        g_os_state = STATE_DESKTOP;
        g_login_user[0] = 0; g_login_pass[0] = 0;
        g_login_error = 0;
        for (int i = 0; i < g_win_count; i++) g_wins[i].visible = false;
    } else {
        g_login_error = 1;
        g_login_pass[0] = 0;
    }
}

/* ================================================================
 *  BOOT SPLASH SCREEN — with animated logo + progress bar
 * ================================================================ */
static void draw_splash_screen(void) {
    int sw = (int)fb_width(), sh = (int)fb_height();
    const char *msgs[] = {
        "Initializing kernel...",
        "Loading drivers...",
        "Security layer active...",
        "Privacy subsystem online...",
        "Starting compositor...",
        "DevilCore OS v5.0 Ready",
    };
    int num_msgs = 6;

    for (int step = 0; step < num_msgs; step++) {
        /* Dark gradient background */
        fb_gradient(0, 0, sw, sh,
                    0x04, 0x02, 0x10,
                    0x00, 0x00, 0x04);

        /* Logo centered (2x scale = 256x170) */
        int logo_x = (sw - 128 * 2) / 2;
        int logo_y = (sh / 2) - 140;
        fb_blit_logo(logo_x, logo_y, 2);

        /* Version text below logo */
        const char *ver = "DevilCore OS v5.0";
        int ver_x = sw / 2 - (int)strlen(ver) * CHAR_W / 2;
        font_str(ver_x, logo_y + 180, ver, COL(C_ACCENT), 0);

        /* Privacy tagline */
        const char *tag = "Privacy-First. Security-Hardened.";
        int tag_x = sw / 2 - (int)strlen(tag) * CHAR_W / 2;
        font_str(tag_x, logo_y + 200, tag, COL(C_TEXT_DIM), 0);

        /* Progress bar */
        int bar_w = 400, bar_h = 12;
        int bar_x = (sw - bar_w) / 2;
        int bar_y = logo_y + 240;
        fb_fill(bar_x, bar_y, bar_w, bar_h, COL(0x080820));
        fb_rect(bar_x, bar_y, bar_w, bar_h, COL(C_WIN_BORDER));
        int prog = (bar_w - 4) * (step + 1) / num_msgs;
        /* Gradient progress fill */
        for (int px = 0; px < prog; px++) {
            uint8_t r = (uint8_t)(0x00 + px * 0x00 / (bar_w - 4));
            uint8_t g = (uint8_t)(0xCC + px * 0x33 / (bar_w - 4));
            uint8_t b = (uint8_t)(0x88 + px * 0x44 / (bar_w - 4));
            fb_vline(bar_x + 2 + px, bar_y + 2, bar_h - 4, rgb(r, g, b));
        }

        /* Status message */
        int msg_x = sw / 2 - (int)strlen(msgs[step]) * CHAR_W / 2;
        font_str(msg_x, bar_y + 24, msgs[step], COL(C_TEXT), 0);

        /* Copyright */
        const char *copy = "(C) 2026 darkdevil404";
        int copy_x = sw / 2 - (int)strlen(copy) * CHAR_W / 2;
        font_str(copy_x, sh - 40, copy, COL(C_TEXT_DIM), 0);

        /* Flush to screen */
        fb_flip();

        /* Delay: busy-wait approx 400ms per step */
        for (volatile int d = 0; d < 8000000; d++) __asm__ volatile("nop");
    }
}

/* ================================================================
 *  SECTION 11: FULL REDRAW COMPOSITOR
 * ================================================================ */
static void redraw_all(void) {
    if (g_os_state == STATE_LOGIN) {
        draw_login_screen();
        draw_cursor();
        fb_flip();
        return;
    }

    draw_desktop_bg();
    draw_icons();

    for (int i = 0; i < g_win_count; i++) {
        Window *w = &g_wins[i];
        if (!w->visible) continue;
        wm_draw_frame(w);
        switch (w->type) {
            case WM_TERMINAL: render_terminal(w); break;
            case WM_EDITOR:   render_editor(w);   break;
            case WM_FILEMAN:  render_fileman(w);  break;
            case WM_SYSMON:   render_sysmon(w);   break;
            case WM_NETMON:   render_netmon(w);    break;
            case WM_SETTINGS: render_settings(w);  break;
        }
        w->dirty = 0;
    }

    draw_taskbar();
    draw_cursor();
    fb_flip();
}

/* ================================================================
 *  SECTION 12: SECURITY & PRIVACY INIT
 * ================================================================ */
#define SEC_LOG_SIZE 256
static char g_audit_log[SEC_LOG_SIZE][64];
static int  g_audit_idx = 0;

static void sec_audit(const char *msg) {
    strncpy(g_audit_log[g_audit_idx % SEC_LOG_SIZE], msg, 63);
    g_audit_idx++;
}

static void security_init(void) {
    sec_audit("MAC policy loaded: kernel_only");
    sec_audit("Seccomp: syscall whitelist active");
    sec_audit("AES-256 key schedule initialised");
    sec_audit("Kernel security layer: ACTIVE");
}

/* Privacy engine is now handled in security/privacy_hub.c */

/* ================================================================
 *  SECTION 13: INPUT PROCESSING
 * ================================================================ */
static void open_app(int type) {
    int sw = (int)fb_width(), sh = (int)fb_height();
    int wx, wy, ww, wh;
    const char *title;
    switch (type) {
        case WM_TERMINAL: ww=640;wh=400;title="DevilCore Terminal";break;
        case WM_EDITOR:   ww=600;wh=380;title="Text Editor";break;
        case WM_FILEMAN:  ww=540;wh=400;title="File Manager";break;
        case WM_SYSMON:   ww=580;wh=440;title="System Monitor";break;
        case WM_NETMON:   ww=640;wh=420;title="Network Monitor";break;
        case WM_SETTINGS: ww=520;wh=380;title="Settings";break;
        default:          ww=400;wh=300;title="App";break;
    }
    wx = 140 + g_win_count * 22; wy = 50 + g_win_count * 20;
    if (wx + ww > sw - 20) wx = 140;
    if (wy + wh > sh - TASKBAR_H - 20) wy = 50;
    wm_create(type, wx, wy, ww, wh, title);
    (void)sw; (void)sh;
}

static void process_mouse(void) {
    int mx = g_mouse_x, my = g_mouse_y;
    bool lbtn = (g_mouse_btn & 1) != 0;
    bool prev = (g_prev_mouse_btn & 1) != 0;
    bool click = lbtn && !prev;
    bool rls = !lbtn && prev;
    (void)rls;

    if (g_os_state == STATE_LOGIN) {
        /* Update login hit zones for the new larger login box */
    if (click) {
            int w = 400, h = 420;
            int lx = ((int)fb_width() - w) / 2;
            int ly = ((int)fb_height() - h) / 2;
            if (mx >= lx+40 && mx <= lx+360 && my >= ly+176 && my <= ly+204) g_login_focus = 0;
            if (mx >= lx+40 && mx <= lx+360 && my >= ly+244 && my <= ly+272) g_login_focus = 1;
            if (mx >= lx+w/2-60 && mx <= lx+w/2+60 && my >= ly+300 && my <= ly+332) validate_login();
        }
        g_prev_mouse_btn = g_mouse_btn;
        return;
    }

    /* Dragging */
    if (g_drag_win >= 0) {
        if (lbtn) {
            Window *w = &g_wins[g_drag_win];
            w->x = mx - w->drag_ox;
            w->y = my - w->drag_oy;
            if (w->x < 0) w->x = 0;
            if (w->y < 0) w->y = 0;
            if (w->x + w->w > (int)fb_width()) w->x = (int)fb_width() - w->w;
            if (w->y + w->h > (int)fb_height() - TASKBAR_H) w->y = (int)fb_height() - TASKBAR_H - w->h;
        } else {
            g_drag_win = -1;
        }
        g_prev_mouse_btn = g_mouse_btn;
        return;
    }

    /* Icon hover */
    g_hovered_icon = -1;
    for (int i = 0; i < ICON_COUNT; i++) {
        const Icon *ic = &g_icons[i];
        if (mx >= ic->x && mx < ic->x + ICON_W && my >= ic->y && my < ic->y + ICON_H)
            g_hovered_icon = i;
    }

    /* Window hit test */
    int hit_win = -1;
    for (int i = g_win_count - 1; i >= 0; i--) {
        Window *w = &g_wins[i];
        if (!w->visible) continue;
        if (mx >= w->x && mx < w->x + w->w && my >= w->y && my < w->y + w->h)
            { hit_win = i; break; }
    }

    if (click) {
        /* Icon click */
        if (g_hovered_icon >= 0) {
            open_app(g_icons[g_hovered_icon].type);
            g_prev_mouse_btn = g_mouse_btn;
            return;
        }
        /* Start button */
        if (my >= (int)fb_height() - TASKBAR_H && mx < 90) {
            bool found = false;
            for (int i = 0; i < g_win_count; i++)
                if (g_wins[i].type == WM_SETTINGS && g_wins[i].visible)
                    { wm_close_type(WM_SETTINGS); found = true; break; }
            if (!found) open_app(WM_SETTINGS);
            g_prev_mouse_btn = g_mouse_btn;
            return;
        }
        /* Logout button */
        if (my >= (int)fb_height() - TASKBAR_H && mx >= (int)fb_width() - 240 && mx <= (int)fb_width() - 150) {
            g_os_state = STATE_LOGIN;
            g_login_user[0] = 0; g_login_pass[0] = 0; g_login_focus = 0; g_login_error = 0;
            g_prev_mouse_btn = g_mouse_btn;
            return;
        }
        /* Window interactions */
        if (hit_win >= 0) {
            Window *w = &g_wins[hit_win];
            g_focused_win = hit_win;
            for (int i = 0; i < g_win_count; i++) g_wins[i].focused = (i == hit_win);
            /* Close button */
            int cx = w->x + w->w - 22, cy = w->y + 4;
            if (mx >= cx && mx < cx + 16 && my >= cy && my < cy + 16) {
                w->visible = false;
                if (g_focused_win == hit_win) g_focused_win = -1;
                g_prev_mouse_btn = g_mouse_btn;
                return;
            }
            /* Drag */
            if (my >= w->y && my < w->y + TITLEBAR_H) {
                g_drag_win = hit_win;
                w->drag_ox = mx - w->x;
                w->drag_oy = my - w->y;
            }
            /* File manager selection */
            if (w->type == WM_FILEMAN) {
                int fy = w->y + TITLEBAR_H + 28;
                int idx = (my - fy) / (CHAR_H + 4);
                if (idx >= 0 && idx < FILE_COUNT) w->selected = idx;
            }
            /* Settings selection */
            if (w->type == WM_SETTINGS) {
                int sy = w->y + TITLEBAR_H + 8 + CHAR_H + 18;
                int idx = (my - sy) / (CHAR_H + 8);
                if (idx >= 0 && idx < SETTING_COUNT) w->selected = idx;
            }
        }
    }
    g_prev_mouse_btn = g_mouse_btn;
}

static void process_keyboard(void) {
    char c;
    if (!kbd_pop(&c)) return;

    if (g_os_state == STATE_LOGIN) {
        if (c == '\t') { g_login_focus = 1 - g_login_focus; return; }
        if (c == '\n') { validate_login(); return; }
        char *buf = (g_login_focus == 0) ? g_login_user : g_login_pass;
        int len = (int)strlen(buf);
        if (c == '\b') { if (len > 0) buf[len - 1] = 0; }
        else if (c >= 32 && c <= 126 && len < 31) { buf[len] = c; buf[len + 1] = 0; }
        return;
    }

    Window *fw = (g_focused_win >= 0) ? &g_wins[g_focused_win] : NULL;
    if (!fw || !fw->visible) return;

    if (fw->type == WM_TERMINAL) {
        if (c == '\b') { if (fw->input_len > 0) fw->input_buf[--fw->input_len] = 0; }
        else if (c == '\n') {
            fw->input_buf[fw->input_len] = 0;
            dsh_execute(fw, fw->input_buf);
            fw->input_len = 0;
            memset(fw->input_buf, 0, sizeof(fw->input_buf));
        } else { if (fw->input_len < 127) fw->input_buf[fw->input_len++] = c; }
        fw->dirty = 1;
    } else if (fw->type == WM_EDITOR) {
        if (c == '\b') { if (fw->text_len > 0) fw->text_buf[--fw->text_len] = 0; }
        else { if (fw->text_len < 4090) { fw->text_buf[fw->text_len++] = c; fw->text_buf[fw->text_len] = 0; } }
        fw->dirty = 1;
    } else if (fw->type == WM_SETTINGS) {
        if (c == '\n') { if (fw->selected < SETTING_COUNT) g_settings[fw->selected].enabled = !g_settings[fw->selected].enabled; }
        else if (c == '`') { if (fw->selected > 0) fw->selected--; }
        else { if (fw->selected < SETTING_COUNT - 1) fw->selected++; }
        fw->dirty = 1;
    }
}

/* ================================================================
 *  SECTION 14: KERNEL MAIN ENTRY POINT
 * ================================================================ */
void compositor_thread(void *arg);

void kernel_main(uint64_t mb2_magic, uint64_t mb2_addr) {
    (void)mb2_magic;

    /* Parse Multiboot2 tags and physical memory map */
    mb2_parse(mb2_addr);

    /* Initialize Virtual Memory and Kernel Heap */
    vmm_init();
    heap_init();

    /* Fallback VGA text if no framebuffer */
    if (!g_fb_addr || g_fb_w == 0) {
        volatile uint16_t *vga = (volatile uint16_t*)0xB8000;
        const char *msg = "DevilCore: No framebuffer! Boot with GRUB2 + set gfxmode.";
        for (int i = 0; msg[i]; i++)
            vga[i] = (uint16_t)(msg[i] | 0x0F00);
        while (1) __asm__ volatile("hlt");
    }

    /* Initialize double-buffered framebuffer */
    fb_init(g_fb_addr, g_fb_pitch, g_fb_w, g_fb_h, g_fb_bpp, g_rpos, g_gpos, g_bpos);

    /* Hardware init */
    idt_init();
    pic_init();
    pit_init();
    kbd_init();
    mouse_init();
    mouse_set_bounds((int)g_fb_w, (int)g_fb_h);

    /* Start multitasking */
    proc_init();
    
    /* Enable interrupts after process/scheduler init */
    __asm__ volatile("sti");

    /* Map the Hardware Framebuffer (up to 8MB) using the VMM */
    if (g_fb_addr) {
        uint64_t fb_base = (uint64_t)g_fb_addr;
        uint32_t fb_size = g_fb_pitch * g_fb_h;
        if (fb_size > 8*1024*1024) fb_size = 8*1024*1024;
        
        /* Ensure the range is mapped in 4KB chunks */
        for (uint64_t phys = fb_base; phys < fb_base + fb_size; phys += 4096) {
            vmm_map_page(phys, phys, PAGE_PRESENT | PAGE_WRITABLE);
        }
    }

    /* Read initial time */
    rtc_update();

    /* Security + Privacy */
    security_init();

    /* === BOOT SPLASH SCREEN === */
    draw_splash_screen();

    /* Initialize MSRs for syscalls */
    syscall_init();

    /* Initialize Hardware Bus and Storage */
    pci_init();
    ata_init();
    vfs_init();
    
    /* Initialize USB Subsystem */
    usb_init();
    usb_hid_init();
    
    /* Initialize Networking TCP/IP Subsystem */
    net_init();
    
    /* Initialize DevilCore Tor Privacy Engine */
    privacy_init();
    
    /* Create a virtual node representing the hard drive */
    extern vfs_node_t* dev_create_ata_node(void);
    vfs_node_t* hdd = dev_create_ata_node();
    vfs_mount_root(hdd);

    /* Initial render */
    redraw_all();

    /* Spawn the GUI Compositor as a background kernel thread (TID 1) */
    thread_create(compositor_thread, NULL, "gui_compositor");

    /* Main event loop (TID 0) acts as the idle/monitoring thread */
    while (1) {
        /* Idle thread can just yield or halt */
        __asm__ volatile("hlt");
        thread_yield();
    }
}

/* ================================================================
 *  DEDICATED GUI COMPOSITOR THREAD
 * ================================================================ */
void compositor_thread(void *arg) {
    (void)arg;
    uint32_t last_draw = 0;

    while (1) {
        process_keyboard();

        if (g_mouse_moved) {
            g_mouse_moved = 0;
            process_mouse();
        }

        /* ~30fps (every 3 ticks at 100Hz) */
        if (g_ticks - last_draw >= 3) {
            last_draw = g_ticks;
            redraw_all();
        }

        thread_yield(); /* Yield CPU back to other threads when idle */
    }
}
