/*
 * DevilCore OS — Graphics & GUI Diagnostics
 * Items 41-50
 */

#include "diag.h"
#include "../gui/framebuffer.h"
#include "../gui/window.h"
#include "../gui/widget.h"
#include "../gui/font.h"
#include "../include/string.h"

/* ================================================================
 * FRAMEBUFFER INFO (Item 41)
 * ================================================================ */
static void diag_gfx_framebuffer(diag_output_t *out) {
    diag_header(out, "Framebuffer Information");

    framebuffer_info_t fb;
    fb_get_info(&fb);

    diag_printf_user(out, "  Framebuffer Address: 0x%016llX\n", fb.address);
    diag_printf_user(out, "  Resolution: %u x %u\n", fb.width, fb.height);
    diag_printf_user(out, "  Color Depth: %u bpp\n", fb.bpp);
    diag_printf_user(out, "  Pitch: %u bytes\n", fb.pitch);
    diag_printf_user(out, "  Pixel Format: %s\n", fb.pixel_format_name);

    /* Calculate memory usage */
    uint64_t fb_size = (uint64_t)fb.pitch * fb.height;
    diag_printf_user(out, "  Framebuffer Size: %.1f MB\n",
        (double)fb_size / (1024.0 * 1024.0));

    if (fb.bpp == 32) {
        diag_printf_user(out, "\n  Channel Masks:\n");
        diag_printf_user(out, "    Red:   0x%08X  Shift: %d  Size: %d\n",
            fb.rmask, fb.rpos, fb.rsize);
        diag_printf_user(out, "    Green: 0x%08X  Shift: %d  Size: %d\n",
            fb.gmask, fb.gpos, fb.gsize);
        diag_printf_user(out, "    Blue:  0x%08X  Shift: %d  Size: %d\n",
            fb.bmask, fb.bpos, fabsize);
    }

    diag_printf_user(out, "\n  Double-buffered: %s\n", fb.double_buffered ? "Yes" : "No");
    if (fb.double_buffered) {
        diag_printf_user(out, "  Backbuffer: 0x%016llX\n", fb.backbuffer);
    }
}

/* ================================================================
 * ACTIVE WINDOWS (Item 42)
 * ================================================================ */
static void diag_gfx_windows(diag_output_t *out) {
    diag_header(out, "Active Windows");

    window_info_t windows[64];
    int nwindows = window_get_list(windows, 64);

    if (nwindows == 0) {
        diag_printf_user(out, "  No windows currently open.\n");
        return;
    }

    diag_printf_user(out, "  #  Handle    Z-Order   X     Y     W      H      Title\n");
    diag_printf_user(out, "  -  --------  --------  ----  ----  -----  -----  ------------------\n");

    for (int i = 0; i < nwindows; i++) {
        diag_printf_user(out, "  %-2d  0x%06X  %-8d  %4d  %4d  %5d  %5d  %s\n",
            i,
            windows[i].handle,
            windows[i].z_order,
            windows[i].x,
            windows[i].y,
            windows[i].width,
            windows[i].height,
            windows[i].title);
    }

    diag_printf_user(out, "\n  Total windows: %d\n", nwindows);
    diag_printf_user(out, "  Active window handle: 0x%06X\n", g_active_window);
    diag_printf_user(out, "  Focus window handle:  0x%06X\n", g_focus_window);
}

/* ================================================================
 * WINDOW MANAGER EVENT QUEUE (Item 43)
 * ================================================================ */
static void diag_gfx_event_queue(diag_output_t *out) {
    diag_header(out, "Window Manager Event Queue");

    wm_event_t events[20];
    int nevents = wm_get_pending_events(events, 20);

    diag_printf_user(out, "  Pending Events: %d\n", nevents);
    diag_printf_user(out, "  Queue Capacity: %d\n", WM_EVENT_QUEUE_SIZE);
    diag_printf_user(out, "\n  Event Details:\n");
    diag_printf_user(out, "    #  Type        Window  Param1  Param2  Time\n");
    diag_printf_user(out, "    -  ----------  ------  ------  ------  ----\n");

    for (int i = 0; i < nevents; i++) {
        const char *etype = "UNKNOWN";
        switch (events[i].type) {
            case WM_EVENT_CREATE:   etype = "CREATE"; break;
            case WM_EVENT_DESTROY:  etype = "DESTROY"; break;
            case WM_EVENT_MOVE:     etype = "MOVE"; break;
            case WM_EVENT_RESIZE:   etype = "RESIZE"; break;
            case WM_EVENT_PAINT:    etype = "PAINT"; break;
            case WM_EVENT_KEY:      etype = "KEY"; break;
            case WM_EVENT_MOUSE:    etype = "MOUSE"; break;
            case WM_EVENT_FOCUS:    etype = "FOCUS"; break;
            case WM_EVENT_CLOSE:    etype = "CLOSE"; break;
        }
        diag_printf_user(out, "    %-2d  %-11s  0x%04X  %-6u  %-6u  %u\n",
            i, etype, events[i].window,
            events[i].param1, events[i].param2,
            events[i].timestamp);
    }

    if (nevents == 0) {
        diag_printf_user(out, "    (queue empty)\n");
    }

    diag_printf_user(out, "\n  Event Rate: %u events/sec\n", g_wm_event_rate);
}

/* ================================================================
 * SCREEN REDRAW REGIONS (Item 44)
 * ================================================================ */
static void diag_gfx_redraw_regions(diag_output_t *out) {
    diag_header(out, "Screen Redraw Regions (Dirty Rectangles)");

    dirty_rect_t dirty[32];
    int ndirty = composer_get_dirty_regions(dirty, 32);

    diag_printf_user(out, "  Dirty Regions: %d\n", ndirty);
    diag_printf_user(out, "  Total Area to Redraw: %u pixels\n", g_composer_dirty_pixels);

    if (ndirty > 0) {
        diag_printf_user(out, "\n  #    X      Y      W      H      Area\n");
        diag_printf_user(out, "  -    ---    ---    ---    ---    ------\n");
        for (int i = 0; i < ndirty; i++) {
            uint32_t area = dirty[i].w * dirty[i].h;
            diag_printf_user(out, "  %-2d  %4d  %4d  %4d  %4d  %6u\n",
                i, dirty[i].x, dirty[i].y, dirty[i].w, dirty[i].h, area);
        }
    }

    diag_printf_user(out, "\n  Composer FPS: %.1f\n", g_composer_fps);
    diag_printf_user(out, "  Compositing Time: %.2f ms\n", g_composer_frame_time_ms);
}

/* ================================================================
 * GUI WIDGETS (Item 45)
 * ================================================================ */
static void diag_gfx_widgets(diag_output_t *out) {
    diag_header(out, "GUI Widgets");

    widget_info_t widgets[128];
    int nwidgets = widget_get_list(widgets, 128);

    diag_printf_user(out, "  Total Widgets: %d\n", nwidgets);
    diag_printf_user(out, "\n  Handle  Type          Parent   X     Y     W      H      Visible\n");
    diag_printf_user(out, "  -------  ------------  -------  ---   ---   ----   ----   --------\n");

    for (int i = 0; i < nwidgets; i++) {
        const char *type = "UNKNOWN";
        switch (widgets[i].type) {
            case WIDGET_BUTTON:      type = "BUTTON"; break;
            case WIDGET_LABEL:       type = "LABEL"; break;
            case WIDGET_TEXTBOX:     type = "TEXTBOX"; break;
            case WIDGET_WINDOW:      type = "WINDOW"; break;
            case WIDGET_PANEL:       type = "PANEL"; break;
            case WIDGET_SCROLLBAR:   type = "SCROLLBAR"; break;
            case WIDGET_CHECKBOX:    type = "CHECKBOX"; break;
            case WIDGET_RADIOBUTTON: type = "RADIO"; break;
            case WIDGET_LISTBOX:     type = "LISTBOX"; break;
            case WIDGET_MENU:        type = "MENU"; break;
        }
        diag_printf_user(out, "  0x%06X  %-12s  0x%06X  %4d  %4d  %5d  %5d  %s\n",
            widgets[i].handle,
            type,
            widgets[i].parent,
            widgets[i].x,
            widgets[i].y,
            widgets[i].width,
            widgets[i].height,
            widgets[i].visible ? "Yes" : "No");
    }

    diag_printf_user(out, "\n  Widget Types:\n");
    diag_printf_user(out, "    BUTTON:       %d\n", g_widget_count[WIDGET_BUTTON]);
    diag_printf_user(out, "    LABEL:        %d\n", g_widget_count[WIDGET_LABEL]);
    diag_printf_user(out, "    WINDOW:       %d\n", g_widget_count[WIDGET_WINDOW]);
    diag_printf_user(out, "    PANEL:        %d\n", g_widget_count[WIDGET_PANEL]);
}

/* ================================================================
 * COMPOSITOR FPS (Item 46)
 * ================================================================ */
static void diag_gfx_fps(diag_output_t *out) {
    diag_header(out, "Compositor Frame Rate");

    diag_printf_user(out, "  Current FPS:          %.1f\n", g_composer_fps);
    diag_printf_user(out, "  Frame Time:           %.2f ms\n", g_composer_frame_time_ms);
    diag_printf_user(out, "  Target FPS:           %d\n", g_target_fps);
    diag_printf_user(out, "  VBlank Interval:      %u ms\n", g_vblank_interval_ms);
    diag_printf_user(out, "  Dropped Frames:       %u\n", g_dropped_frames);
    diag_printf_user(out, "  Total Frames Rendered: %u\n", g_total_frames);

    double fps_pct = g_target_fps ? (double)g_composer_fps * 100.0 / (double)g_target_fps : 0.0;
    diag_printf_user(out, "\n  Performance: %.1f%% of target\n", fps_pct);
    if (g_composer_fps >= g_target_fps * 0.95) {
        diag_printf_user(out, "  Status: EXCELLENT\n");
    } else if (g_composer_fps >= g_target_fps * 0.8) {
        diag_printf_user(out, "  Status: GOOD\n");
    } else if (g_composer_fps >= g_target_fps * 0.5) {
        diag_printf_user(out, "  Status: FAIR\n");
    } else {
        diag_printf_user(out, "  Status: POOR\n");
    }
}

/* ================================================================
 * MOUSE CURSOR (Item 47)
 * ================================================================ */
static void diag_gfx_mouse(diag_output_t *out) {
    diag_header(out, "Mouse Cursor");

    mouse_state_t mouse;
    mouse_get_state(&mouse);

    diag_printf_user(out, "  Position: (%d, %d)\n", mouse.x, mouse.y);
    diag_printf_user(out, "  Relative Delta: (%+d, %+d)\n", mouse.dx, mouse.dy);
    diag_printf_user(out, "  Buttons: ");
    if (mouse.buttons & MOUSE_BUTTON_LEFT)   diag_printf_user(out, "L");
    if (mouse.buttons & MOUSE_BUTTON_RIGHT)  diag_printf_user(out, "R");
    if (mouse.buttons & MOUSE_BUTTON_MIDDLE) diag_printf_user(out, "M");
    diag_printf_user(out, "\n");

    diag_printf_user(out, "  Cursor Shape: %s\n",
        mouse.cursor_shape == CURSOR_ARROW ? "Arrow" :
        mouse.cursor_shape == CURSOR_HAND ? "Hand" :
        mouse.cursor_shape == CURSOR_IBEAM ? "I-Beam" :
        mouse.cursor_shape == CURSOR_WAIT ? "Wait" : "Unknown");

    diag_printf_user(out, "  Cursor Visible: %s\n", mouse.visible ? "Yes" : "No");
    diag_printf_user(out, "  Hitbox: %d x %d pixels\n", mouse.hitbox_w, mouse.hitbox_h);
    diag_printf_user(out, "  Acceleration: %.2f\n", mouse.acceleration);
    diag_printf_user(out, "  Sample Rate: %u Hz\n", mouse.sample_rate);
}

/* ================================================================
 * FONT CACHE (Item 48)
 * ================================================================ */
static void diag_gfx_font_cache(diag_output_t *out) {
    diag_header(out, "Font Cache");

    font_cache_stats_t font_stats;
    font_get_cache_stats(&font_stats);

    diag_printf_user(out, "  Active Font: %s %dpt\n", font_stats.active_name, font_stats.active_size);
    diag_printf_user(out, "  Glyph Cache Entries: %u\n", font_stats.glyph_entries);
    diag_printf_user(out, "  Glyph Cache Size:   %u KB\n", font_stats.cache_size_kb);
    diag_printf_user(out, "  Glyph Cache Hits:   %llu\n", font_stats.hits);
    diag_printf_user(out, "  Glyph Cache Misses: %llu\n", font_stats.misses);
    diag_printf_user(out, "  Hit Rate:           %.2f%%\n",
        font_stats.hits + font_stats.misses ?
        (double)font_stats.hits * 100.0 / (double)(font_stats.hits + font_stats.misses) : 0.0);

    diag_printf_user(out, "\n  Loaded Fonts:\n");
    for (int i = 0; i < font_stats.num_fonts; i++) {
        diag_printf_user(out, "    %s %dpt (ID: 0x%02X) - %u glyphs\n",
            font_stats.fonts[i].name,
            font_stats.fonts[i].size,
            font_stats.fonts[i].id,
            font_stats.fonts[i].num_glyphs);
    }
}

/* ================================================================
 * GRAPHICAL SURFACES (Item 49)
 * ================================================================ */
static void diag_gfx_surfaces(diag_output_t *out) {
    diag_header(out, "Graphical Surfaces");

    surface_info_t surfaces[32];
    int nsurf = graphics_get_surfaces(surfaces, 32);

    diag_printf_user(out, "  Active Surfaces: %d\n", nsurf);
    diag_printf_user(out, "\n  ID    Type        Width   Height  BPP    Pitch   Address\n");
    diag_printf_user(out, "  ----- ---------   -----   ------  ---  -------  ----------------\n");

    for (int i = 0; i < nsurf; i++) {
        const char *stype = "UNKNOWN";
        switch (surfaces[i].type) {
            case SURFACE_FRAMEBUFFER: stype = "Framebuffer"; break;
            case SURFACE_TEXTURE:     stype = "Texture"; break;
            case SURFACE_OFFSCREEN:   stype = "Offscreen"; break;
            case SURFACE_CURSOR:      stype = "Cursor"; break;
            case SURFACE_WINDOW:      stype = "Window"; break;
        }
        diag_printf_user(out, "  0x%03X  %-11s  %5d  %6d  %3d  %7u  0x%016llX\n",
            surfaces[i].id,
            stype,
            surfaces[i].width,
            surfaces[i].height,
            surfaces[i].bpp,
            surfaces[i].pitch,
            surfaces[i].address);
    }
}

/* ================================================================
 * DOUBLE-BUFFER SWAP CHAIN (Item 50)
 * ================================================================ */
static void diag_gfx_swap_chain(diag_output_t *out) {
    diag_header(out, "Double-Buffer Swap Chain Status");

    swap_chain_info_t swap;
    graphics_get_swap_chain_info(&swap);

    diag_printf_user(out, "  Swap Chain Enabled:   %s\n", swap.enabled ? "Yes" : "No");
    diag_printf_user(out, "  Number of Buffers:    %u\n", swap.buffer_count);
    diag_printf_user(out, "  Current Buffer:       %u (0=front, 1=back)\n", swap.current_buffer);
    diag_printf_user(out, "  Buffer Size:          %u x %u x %u bpp\n",
        swap.width, swap.height, swap.bpp);
    diag_printf_user(out, "  Buffer Addresses:\n");
    for (int i = 0; i < swap.buffer_count; i++) {
        diag_printf_user(out, "    Buffer %d: 0x%016llX\n", i, swap.buffer_addrs[i]);
    }
    diag_printf_user(out, "\n  VSync: %s\n", swap.vsync_enabled ? "Enabled" : "Disabled");
    diag_printf_user(out, "  Triple Buffering: %s\n", swap.triple_buffered ? "Yes" : "No");
    diag_printf_user(out, "  Swap Pending: %s\n", swap.swap_pending ? "Yes" : "No");
    diag_printf_user(out, "  Last Swap Time: %llu ns\n", swap.last_swap_timestamp);
}

/* ================================================================
 * GRAPHICS DISPATCHER
 * ================================================================ */
void diag_graphics_impl(uint64_t subcmd, diag_output_t *out) {
    switch (subcmd) {
        case DIAG_GFX_FRAMEBUFFER:
            diag_gfx_framebuffer(out);
            break;
        case DIAG_GFX_WINDOWS:
            diag_gfx_windows(out);
            break;
        case DIAG_GFX_EVENT_QUEUE:
            diag_gfx_event_queue(out);
            break;
        case DIAG_GFX_REDRAW_REGIONS:
            diag_gfx_redraw_regions(out);
            break;
        case DIAG_GFX_WIDGETS:
            diag_gfx_widgets(out);
            break;
        case DIAG_GFX_FPS:
            diag_gfx_fps(out);
            break;
        case DIAG_GFX_MOUSE:
            diag_gfx_mouse(out);
            break;
        case DIAG_GFX_FONT_CACHE:
            diag_gfx_font_cache(out);
            break;
        case DIAG_GFX_SURFACES:
            diag_gfx_surfaces(out);
            break;
        case DIAG_GFX_SWAP_CHAIN:
            diag_gfx_swap_chain(out);
            break;
        case 0: /* All graphics diagnostics */
            diag_gfx_framebuffer(out);
            diag_gfx_windows(out);
            diag_gfx_event_queue(out);
            diag_gfx_redraw_regions(out);
            diag_gfx_widgets(out);
            diag_gfx_fps(out);
            diag_gfx_mouse(out);
            diag_gfx_font_cache(out);
            diag_gfx_surfaces(out);
            diag_gfx_swap_chain(out);
            break;
        default:
            diag_header(out, "Graphics Diagnostics");
            diag_printf_user(out, "  Unknown subcommand: %lu\n", subcmd);
            break;
    }
}
