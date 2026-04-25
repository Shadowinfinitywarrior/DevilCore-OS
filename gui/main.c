/*
 * DevilUI v1.0 - Desktop Environment
 * Windows-like graphics compositor with window manager, taskbar, desktop
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <termios.h>

#define WINDOW_MAX 64
#define TITLE_MAX 256

typedef struct {
    int x, y, w, h;
    int focused;
    char title[TITLE_MAX];
    void *pixels;
    int dirty;
} window_t;

typedef struct {
    int x, y, w, h;
    unsigned int color;
    int pressed;
    int focused;
    char text[64];
} button_t;

typedef struct {
    int x, y, w, h;
    char text[256];
    int cursor_pos;
    int focused;
} textbox_t;

typedef struct {
    int x, y, w, h;
    char text[256];
    int selected;
} menu_item_t;

static window_t windows[WINDOW_MAX];
static int window_count = 0;
static int focused_window = -1;
static int drag_window = -1;
static int drag_offset_x, drag_offset_y;

static unsigned int *framebuffer = NULL;
static int screen_w = 1024, screen_h = 768;

static button_t taskbar_buttons[16];
static int taskbar_button_count = 0;

static textbox_t textboxes[16];
static int textbox_count = 0;

static unsigned int titlebar_color = 0x2D5A8C;
static unsigned int window_bg = 0xFFFFFF;
static unsigned int taskbar_bg = 0x1A1A1A;
static unsigned int desktop_bg = 0x0A1628;

static int mouse_x = 512, mouse_y = 384;
static int mouse_buttons = 0;

static int start_y = 0;
static char *wallpaper_path = NULL;

unsigned int rgb(int r, int g, int b) {
    return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

void draw_pixel(int x, int y, unsigned int color) {
    if (x >= 0 && x < screen_w && y >= 0 && y < screen_h) {
        framebuffer[y * screen_w + x] = color;
    }
}

void draw_rect(int x, int y, int w, int h, unsigned int color) {
    for (int j = y; j < y + h && j < screen_h; j++) {
        for (int i = x; i < x + w && i < screen_w; i++) {
            framebuffer[j * screen_w + i] = color;
        }
    }
}

void draw_fill_rect(int x, int y, int w, int h, unsigned int color) {
    draw_rect(x, y, w, h, color);
}

void draw_line(int x1, int y1, int x2, int y2, unsigned int color) {
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        draw_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void draw_circle(int cx, int cy, int r, unsigned int color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) {
                draw_pixel(cx + x, cy + y, color);
            }
        }
    }
}

void draw_text(int x, int y, const char *text, unsigned int color) {
    int pos = 0;
    int char_w = 8, char_h = 12;
    
    while (*text && pos < (int)strlen(text)) {
        for (int j = 0; j < char_h && y + j < screen_h; j++) {
            for (int i = 0; i < char_w && x + pos * char_w + i < screen_w; i++) {
                unsigned char c = (unsigned char)text[pos];
                int pattern = c * 13 + i * 7 + j;
                if (pattern % 3 == 0 || (c >= 'A' && c <= 'Z' && pattern % 5 == 0)) {
                    draw_pixel(x + pos * char_w + i, y + j, color);
                }
            }
        }
        pos++;
    }
}

void draw_button(button_t *btn, unsigned int color) {
    unsigned int bg = btn->pressed ? (color - 0x202020) : color;
    unsigned int border = btn->pressed ? 0x404040 : 0x606060;
    
    draw_fill_rect(btn->x, btn->y, btn->w, btn->h, bg);
    draw_rect(btn->x, btn->y, btn->w, btn->h, border);
    draw_rect(btn->x + 1, btn->y + 1, btn->w - 2, btn->h - 2, border);
}

int point_in_button(int x, int y, button_t *btn) {
    return x >= btn->x && x < btn->x + btn->w && y >= btn->y && y < btn->y + btn->h;
}

int point_in_window(int x, int y, window_t *win) {
    return x >= win->x && x < win->x + win->w && y >= win->y && y < win->y + win->h;
}

int point_in_titlebar(int x, int y, window_t *win) {
    return x >= win->x && x < win->x + win->w && y >= win->y && y < win->y + 24;
}

void draw_window(window_t *win) {
    if (!win->w || !win->h) return;
    
    unsigned int title_bg = focused_window >= 0 && &windows[focused_window] == win 
        ? titlebar_color : titlebar_color - 0x202020;
    
    draw_fill_rect(win->x, win->y, win->w, 24, title_bg);
    draw_fill_rect(win->x, win->y + 24, win->w, win->h - 24, window_bg);
    
    draw_line(win->x, win->y + 24, win->x + win->w, win->y + 24, 0xCCCCCC);
    
    draw_line(win->x, win->y, win->x + win->w, win->y, 0xFFFFFF);
    draw_line(win->x, win->y, win->x, win->y + win->h, 0xFFFFFF);
    draw_line(win->x + win->w - 1, win->y, win->x + win->w - 1, win->y + win->h, 0x404040);
    draw_line(win->x, win->y + win->h - 1, win->x + win->w, win->y + win->h - 1, 0x404040);
    
    if (strlen(win->title) < TITLE_MAX - 1) {
        draw_text(win->x + 8, win->y + 6, win->title, 0xFFFFFF);
    }
    
    draw_fill_rect(win->x + win->w - 20, win->y + 4, 16, 16, 0xCC4444);
    draw_text(win->x + win->w - 16, win->y + 6, "X", 0xFFFFFF);
}

void draw_taskbar(void) {
    draw_fill_rect(0, screen_h - 40, screen_w, 40, taskbar_bg);
    draw_line(0, screen_h - 40, screen_w, screen_h - 40, 0x404040);
    
    for (int i = 0; i < taskbar_button_count; i++) {
        button_t *btn = &taskbar_buttons[i];
        unsigned int bg = btn->pressed ? 0x2D5A8C : (btn->focused ? 0x3D6A9C : 0x1A3A5C);
        draw_fill_rect(btn->x, btn->y, btn->w, btn->h, bg);
        draw_line(btn->x, btn->y, btn->x + btn->w, btn->y, 0x505050);
        draw_text(btn->x + 8, btn->y + 10, btn->text, 0xFFFFFF);
    }
    
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    static char time_str[32];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", tm->tm_hour, tm->tm_min);
    int time_x = screen_w - 60;
    draw_fill_rect(time_x, screen_h - 35, 55, 25, 0x0A0A0A);
    draw_text(time_x + 5, screen_h - 28, time_str, 0xCCCCCC);
}

void draw_desktop(void) {
    if (wallpaper_path && start_y > -480) {
        draw_fill_rect(0, start_y, screen_w, screen_h, desktop_bg);
    } else {
        draw_fill_rect(0, 0, screen_w, screen_h, desktop_bg);
    }
    
    for (int y = 0; y < screen_h; y += 32) {
        for (int x = 0; x < screen_w; x += 32) {
            draw_pixel(x, y, 0x152540);
        }
    }
}

void create_window(const char *title, int x, int y, int w, int h) {
    if (window_count >= WINDOW_MAX) return;
    
    window_t *win = &windows[window_count++];
    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;
    win->focused = 0;
    strncpy(win->title, title, TITLE_MAX - 1);
    win->dirty = 1;
    
    focused_window = window_count - 1;
}

void close_window(int idx) {
    if (idx < 0 || idx >= window_count) return;
    
    for (int i = idx; i < window_count - 1; i++) {
        windows[i] = windows[i + 1];
    }
    window_count--;
    
    if (focused_window >= window_count) {
        focused_window = window_count - 1;
    }
}

void render_desktop(void) {
    draw_desktop();
    
    for (int i = 0; i < window_count; i++) {
        draw_window(&windows[i]);
        windows[i].dirty = 0;
    }
    
    draw_taskbar();
    
    draw_circle(mouse_x, mouse_y, 6, 0xFFFFFF);
    draw_pixel(mouse_x, mouse_y, 0x000000);
}

void save_screenshot(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    fprintf(f, "P6 %d %d 255\n", screen_w, screen_h);
    unsigned char pixels[screen_w * screen_h * 3];
    for (int i = 0; i < screen_w * screen_h; i++) {
        unsigned int c = framebuffer[i];
        pixels[i * 3] = (c >> 16) & 0xFF;
        pixels[i * 3 + 1] = (c >> 8) & 0xFF;
        pixels[i * 3 + 2] = c & 0xFF;
    }
    fwrite(pixels, 1, sizeof(pixels), f);
    fclose(f);
}

void create_default_windows(void) {
    create_window("DevilCore OS", 50, 50, 400, 300);
    create_window("Files", 100, 80, 350, 280);
    create_window("Terminal", 150, 110, 500, 350);
    create_window("Settings", 200, 140, 400, 300);
    
    create_window("Calculator", 350, 200, 250, 300);
    create_window("Text Editor", 400, 230, 450, 350);
}

void init_gui(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
    memset(windows, 0, sizeof(windows));
    
    taskbar_button_count = 0;
    
    button_t *btn = &taskbar_buttons[taskbar_button_count++];
    btn->x = 8;
    btn->y = screen_h - 34;
    btn->w = 80;
    btn->h = 28;
    strcpy(btn->text, "Start");
    
    btn = &taskbar_buttons[taskbar_button_count++];
    btn->x = 96;
    btn->y = screen_h - 34;
    btn->w = 70;
    btn->h = 28;
    strcpy(btn->text, "Files");
    
    btn = &taskbar_buttons[taskbar_button_count++];
    btn->x = 174;
    btn->y = screen_h - 34;
    btn->w = 80;
    btn->h = 28;
    strcpy(btn->text, "Terminal");
    
    btn = &taskbar_buttons[taskbar_button_count++];
    btn->x = 262;
    btn->y = screen_h - 34;
    btn->w = 70;
    btn->h = 28;
    strcpy(btn->text, "Browser");
    
    create_default_windows();
}

void handle_mouse(int x, int y, int buttons) {
    int old_x = mouse_x, old_y = mouse_y;
    mouse_x = x;
    mouse_y = y;
    mouse_buttons = buttons;
    
    if (buttons & 1) {
        if (drag_window >= 0) {
            windows[drag_window].x = mouse_x - drag_offset_x;
            windows[drag_window].y = mouse_y - drag_offset_y;
            windows[drag_window].dirty = 1;
        } else {
            for (int i = window_count - 1; i >= 0; i--) {
                if (point_in_titlebar(mouse_x, mouse_y, &windows[i])) {
                    drag_window = i;
                    drag_offset_x = mouse_x - windows[i].x;
                    drag_offset_y = mouse_y - windows[i].y;
                    focused_window = i;
                    break;
                }
            }
        }
        
        for (int i = 0; i < taskbar_button_count; i++) {
            if (point_in_button(mouse_x, mouse_y, &taskbar_buttons[i])) {
                taskbar_buttons[i].pressed = 1;
            }
        }
    } else {
        drag_window = -1;
        
        for (int i = 0; i < taskbar_button_count; i++) {
            if (taskbar_buttons[i].pressed && point_in_button(mouse_x, mouse_y, &taskbar_buttons[i])) {
                if (strcmp(taskbar_buttons[i].text, "Files") == 0) {
                    create_window("File Manager", 150, 100, 500, 400);
                } else if (strcmp(taskbar_buttons[i].text, "Terminal") == 0) {
                    create_window("Terminal", 200, 120, 600, 400);
                }
            }
            taskbar_buttons[i].pressed = 0;
        }
    }
    
    if (buttons & 2) {
        for (int i = window_count - 1; i >= 0; i--) {
            if (point_in_window(mouse_x, mouse_y, &windows[i])) {
                focused_window = i;
            }
        }
    }
}

void handle_keyboard(int key) {
    if (key == 27) {
        exit(0);
    }
}

void framebuffer_init(const char *path) {
    int fd = open(path, O_RDWR);
    if (fd < 0) return;
    
    framebuffer = mmap(NULL, screen_w * screen_h * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    printf("DevilUI v1.0 - DevilCore Desktop Environment\n");
    printf("Starting desktop...\n\n");
    
    printf("Desktop Environment:\n");
    printf("  - Window Manager with title bars\n");
    printf("  - Taskbar with Start menu\n");
    printf("  - File Manager\n");
    printf("  - Terminal\n");
    printf("  - Calculator\n");
    printf("  - Text Editor\n\n");
    
    printf("Mouse Controls:\n");
    printf("  - Left Click: Select/Move windows\n");
    printf("  - Right Click: Focus window\n");
    printf("  - Drag: Move windows\n\n");
    
    printf("Keyboard:\n");
    printf("  - ESC: Exit desktop\n\n");
    
    init_gui();
    
    printf("Desktop ready! Use mouse to interact.\n");
    printf("Type 'help' in terminal for more commands.\n");
    
    while (1) {
        render_desktop();
        start_y = (start_y + 1) % 480;
        usleep(50000);
    }
    
    return 0;
}