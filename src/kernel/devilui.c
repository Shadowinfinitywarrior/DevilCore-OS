#include "devilui.h"
#include "memory.h"
#include "slab.h"
#include <string.h>

extern void kprintf(const char *fmt, ...);

static uint32_t next_widget_id = 1;
static theme_t *current_theme = NULL;
static widget_t *widget_list_head = NULL;
widget_t *focused_widget = NULL;
widget_t *hovered_widget = NULL;

// Initialize DevilUI
void devilui_init(void) {
    kprintf("Initializing DevilUI...\n");
    
    // Initialize default theme
    static theme_t default_theme;
    theme_init_default(&default_theme);
    theme_apply(&default_theme);
    
    // Initialize animation system
    animation_init();
    
    // Initialize event loop
    event_loop_init();
    
    kprintf("DevilUI initialized\n");
}

void devilui_shutdown(void) {
    // Clean up all widgets
    while (widget_list_head) {
        widget_destroy(widget_list_head);
    }
}

// Widget management
widget_t *widget_create(widget_type_t type, const char *name, 
                         int32_t x, int32_t y, uint32_t width, uint32_t height) {
    widget_t *w = slab_alloc(cache_256);  // Widgets fit in 256 bytes
    if (!w) {
        kprintf("Failed to allocate widget\n");
        return NULL;
    }
    
    memset(w, 0, sizeof(widget_t));
    
    w->type = type;
    w->id = next_widget_id++;
    strncpy(w->name, name, 63);
    w->name[63] = '\0';
    
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    
    w->min_width = 0;
    w->min_height = 0;
    w->max_width = UINT32_MAX;
    w->max_height = UINT32_MAX;
    
    w->layout = LAYOUT_NONE;
    w->h_policy = SIZE_PREFERRED;
    w->v_policy = SIZE_PREFERRED;
    
    w->visible = true;
    w->enabled = true;
    
    w->theme = current_theme;
    w->style = NULL;
    
    // Add to global widget list
    w->next = widget_list_head;
    if (widget_list_head) widget_list_head->prev = w;
    widget_list_head = w;
    
    return w;
}

void widget_destroy(widget_t *w) {
    if (!w) return;
    
    // Remove from parent
    if (w->parent) {
        widget_remove_child(w->parent, w);
    }
    
    // Destroy children
    while (w->child_count > 0) {
        widget_destroy(w->children[0]);
    }
    
    if (w->children) kfree(w->children);
    
    // Remove from global list
    if (w->prev) w->prev->next = w->next;
    else widget_list_head = w->next;
    if (w->next) w->next->prev = w->prev;
    
    // Clear focus/hover if this widget had it
    if (focused_widget == w) focused_widget = NULL;
    if (hovered_widget == w) hovered_widget = NULL;
    
    // Call vtable destructor
    if (w->vtable && w->vtable->destroy) {
        w->vtable->destroy(w);
    }
    
    slab_free(cache_256, w);
}

void widget_add_child(widget_t *parent, widget_t *child) {
    if (!parent || !child) return;
    
    // Allocate/expand children array
    if (parent->child_count >= parent->child_capacity) {
        uint32_t new_capacity = parent->child_capacity ? parent->child_capacity * 2 : 4;
        widget_t **new_children = kmalloc(new_capacity * sizeof(widget_t*));
        if (!new_children) return;
        
        if (parent->children) {
            memcpy(new_children, parent->children, 
                   parent->child_count * sizeof(widget_t*));
            kfree(parent->children);
        }
        
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }
    
    parent->children[parent->child_count++] = child;
    child->parent = parent;
    
    // Trigger layout
    widget_layout(parent);
}

void widget_remove_child(widget_t *parent, widget_t *child) {
    if (!parent || !child) return;
    
    // Find child in array
    for (uint32_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            // Shift remaining children
            for (uint32_t j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->child_count--;
            child->parent = NULL;
            return;
        }
    }
}

void widget_show(widget_t *w) {
    if (!w || w->visible) return;
    w->visible = true;
    widget_invalidate(w);
}

void widget_hide(widget_t *w) {
    if (!w || !w->visible) return;
    w->visible = false;
    widget_invalidate(w);
}

void widget_set_focus(widget_t *w) {
    if (!w || w == focused_widget) return;
    
    // Clear old focus
    if (focused_widget) {
        focused_widget->focused = false;
        if (focused_widget->on_focus_out) {
            focused_widget->on_focus_out(focused_widget);
        }
    }
    
    // Set new focus
    focused_widget = w;
    w->focused = true;
    
    if (w->on_focus_in) {
        w->on_focus_in(w);
    }
    
    widget_invalidate(w);
}

void widget_clear_focus(widget_t *w) {
    if (!w || focused_widget != w) return;
    
    w->focused = false;
    focused_widget = NULL;
    
    if (w->on_focus_out) {
        w->on_focus_out(w);
    }
    
    widget_invalidate(w);
}

void widget_invalidate(widget_t *w) {
    if (!w || !g_compositor) return;
    
    rect_t damage = {w->x, w->y, w->width, w->height};
    compositor_add_damage(g_compositor, &damage);
}

void widget_layout(widget_t *w) {
    if (!w) return;
    
    // Call vtable layout
    if (w->vtable && w->vtable->layout) {
        w->vtable->layout(w);
    }
    
    // Simple layout implementation
    if (w->layout == LAYOUT_VERTICAL) {
        int32_t y = w->y + w->padding_top;
        int32_t x = w->x + w->padding_left;
        uint32_t available_width = w->width - w->padding_left - w->padding_right;
        
        for (uint32_t i = 0; i < w->child_count; i++) {
            widget_t *child = w->children[i];
            if (!child->visible) continue;
            
            child->x = x + child->margin_left;
            child->y = y + child->margin_top;
            
            if (child->h_policy == SIZE_EXPANDING) {
                child->width = available_width - child->margin_left - child->margin_right;
            }
            
            y += child->height + child->margin_top + child->margin_bottom + w->spacing;
        }
    } else if (w->layout == LAYOUT_HORIZONTAL) {
        int32_t x = w->x + w->padding_left;
        int32_t y = w->y + w->padding_top;
        uint32_t available_height = w->height - w->padding_top - w->padding_bottom;
        
        for (uint32_t i = 0; i < w->child_count; i++) {
            widget_t *child = w->children[i];
            if (!child->visible) continue;
            
            child->x = x + child->margin_left;
            child->y = y + child->margin_top;
            
            if (child->v_policy == SIZE_EXPANDING) {
                child->height = available_height - child->margin_top - child->margin_bottom;
            }
            
            x += child->width + child->margin_left + child->margin_right + w->spacing;
        }
    }
    
    // Recursively layout children
    for (uint32_t i = 0; i < w->child_count; i++) {
        widget_layout(w->children[i]);
    }
}

void widget_paint(widget_t *w, painter_t *p) {
    if (!w || !p || !w->visible) return;
    
    painter_save(p);
    
    // Apply widget transform
    painter_translate(p, w->x, w->y);
    
    // Set clip to widget bounds
    rect_t clip = {0, 0, w->width, w->height};
    painter_clip_rect(p, &clip);
    
    // Call vtable paint
    if (w->vtable && w->vtable->paint) {
        w->vtable->paint(w, p);
    }
    
    // Call custom paint handler
    if (w->on_paint) {
        w->on_paint(w, p);
    }
    
    // Paint children
    for (uint32_t i = 0; i < w->child_count; i++) {
        widget_paint(w->children[i], p);
    }
    
    painter_restore(p);
}

// Hit testing
widget_t *widget_hit_test(widget_t *root, int32_t x, int32_t y) {
    if (!root || !root->visible) return NULL;
    
    // Test children first (front to back for z-order)
    for (int32_t i = (int32_t)root->child_count - 1; i >= 0; i--) {
        widget_t *hit = widget_hit_test(root->children[i], x, y);
        if (hit) return hit;
    }
    
    // Test self
    if (widget_contains(root, x, y)) {
        return root;
    }
    
    return NULL;
}

bool widget_contains(widget_t *w, int32_t x, int32_t y) {
    if (!w) return false;
    return x >= w->x && x < (int32_t)(w->x + w->width) &&
           y >= w->y && y < (int32_t)(w->y + w->height);
}

// Layout helpers
void widget_set_layout(widget_t *w, layout_type_t layout) {
    if (!w) return;
    w->layout = layout;
    widget_layout(w);
}

void widget_set_size_policy(widget_t *w, size_policy_t h, size_policy_t v) {
    if (!w) return;
    w->h_policy = h;
    w->v_policy = v;
}

void widget_set_margins(widget_t *w, uint32_t top, uint32_t right, 
                         uint32_t bottom, uint32_t left) {
    if (!w) return;
    w->margin_top = top;
    w->margin_right = right;
    w->margin_bottom = bottom;
    w->margin_left = left;
}

void widget_set_padding(widget_t *w, uint32_t top, uint32_t right,
                         uint32_t bottom, uint32_t left) {
    if (!w) return;
    w->padding_top = top;
    w->padding_right = right;
    w->padding_bottom = bottom;
    w->padding_left = left;
}

// Window implementation
static void window_paint(widget_t *w, painter_t *p) {
    widget_style_t *style = w->style ? w->style : &w->theme->window_style;
    rect_t rect = {0, 0, w->width, w->height};
    
    // Draw shadow
    if (style->normal.shadow_count > 0) {
        // Shadow rendering is complex, for now just a simple offset rect
        // painter_draw_shadow(p, &rect, &style->normal.shadows[0], style->normal.border.radius);
    }
    
    // Draw background
    painter_set_fill(p, style->normal.background);
    painter_fill_round_rect(p, &rect, style->normal.border.radius);
    
    // Draw border
    painter_set_stroke(p, style->normal.border.color, style->normal.border.width);
    painter_stroke_round_rect(p, &rect, style->normal.border.radius);
    
    // Draw title bar (simplified)
    rect_t title_rect = {0, 0, w->width, 24};
    painter_set_fill(p, w->theme->accent);
    painter_fill_round_rect(p, &title_rect, style->normal.border.radius);
    
    // Draw title text
    painter_set_fill(p, w->theme->on_background);
    painter_draw_text_rect(p, &title_rect, w->name, TEXT_ALIGN_CENTER);
}

static widget_vtable_t window_vtable = {
    .paint = window_paint,
    .layout = NULL, // Use default layout
    .measure = NULL,
    .hit_test = NULL,
    .find_child = NULL,
    .destroy = NULL,
    .on_event = NULL
};

widget_t *window_create(const char *title, int32_t x, int32_t y, uint32_t w, uint32_t h) {
    widget_t *win = widget_create(WIDGET_WINDOW, title, x, y, w, h);
    if (win) {
        win->vtable = &window_vtable;
        win->padding_top = 28; // Title bar + some padding
        win->padding_left = 4;
        win->padding_right = 4;
        win->padding_bottom = 4;
    }
    return win;
}

// Button implementation
static void button_paint(widget_t *w, painter_t *p) {
    widget_style_t *style = w->style ? w->style : &w->theme->button_style;
    widget_style_state_t *state = &style->normal;
    
    if (w->pressed) state = &style->pressed;
    else if (w->hovered) state = &style->hover;
    
    rect_t rect = {0, 0, w->width, w->height};
    painter_set_fill(p, state->background);
    painter_fill_round_rect(p, &rect, state->border.radius);
    
    painter_set_stroke(p, state->border.color, state->border.width);
    painter_stroke_round_rect(p, &rect, state->border.radius);
    
    painter_set_fill(p, state->text_color);
    painter_draw_text_rect(p, &rect, w->name, TEXT_ALIGN_CENTER);
}

static widget_vtable_t button_vtable = {
    .paint = button_paint,
    .layout = NULL,
    .measure = NULL,
    .hit_test = NULL,
    .find_child = NULL,
    .destroy = NULL,
    .on_event = NULL
};

widget_t *button_create(const char *label, widget_t *parent) {
    widget_t *btn = widget_create(WIDGET_BUTTON, label, 0, 0, 80, 24);
    if (btn) {
        btn->vtable = &button_vtable;
        if (parent) widget_add_child(parent, btn);
    }
    return btn;
}

// Label implementation
static void label_paint(widget_t *w, painter_t *p) {
    rect_t rect = {0, 0, w->width, w->height};
    painter_set_fill(p, w->theme->on_surface);
    painter_draw_text_rect(p, &rect, w->name, TEXT_ALIGN_LEFT);
}

static widget_vtable_t label_vtable = {
    .paint = label_paint,
    .layout = NULL,
    .measure = NULL,
    .hit_test = NULL,
    .find_child = NULL,
    .destroy = NULL,
    .on_event = NULL
};

widget_t *label_create(const char *text, widget_t *parent) {
    widget_t *lbl = widget_create(WIDGET_LABEL, text, 0, 0, 100, 16);
    if (lbl) {
        lbl->vtable = &label_vtable;
        if (parent) widget_add_child(parent, lbl);
    }
    return lbl;
}

void theme_init_default(theme_t *theme) {
    memset(theme, 0, sizeof(theme_t));
    strncpy(theme->name, "Default", 63);
    
    // Base colors (dark theme)
    theme->accent = color_rgb(0x66, 0x99, 0xFF);
    theme->background = color_rgb(0x1a, 0x1a, 0x1a);
    theme->surface = color_rgb(0x2d, 0x2d, 0x2d);
    theme->on_background = color_rgb(0xff, 0xff, 0xff);
    theme->on_surface = color_rgb(0xe0, 0xe0, 0xe0);
    theme->error = color_rgb(0xcf, 0x66, 0x79);
    theme->success = color_rgb(0x98, 0xc3, 0x79);
    theme->warning = color_rgb(0xe5, 0xc0, 0x7b);
    
    // Button style
    theme->button_style.normal.background = color_rgb(0x40, 0x40, 0x40);
    theme->button_style.normal.text_color = theme->on_surface;
    theme->button_style.normal.border.width = 1;
    theme->button_style.normal.border.color = color_rgb(0x60, 0x60, 0x60);
    theme->button_style.normal.border.radius = 4;
    theme->button_style.hover.background = color_rgb(0x50, 0x50, 0x50);
    theme->button_style.pressed.background = color_rgb(0x30, 0x30, 0x30);
    
    // Window style
    theme->window_style.normal.background = theme->surface;
    theme->window_style.normal.border.width = 1;
    theme->window_style.normal.border.color = color_rgb(0x50, 0x50, 0x50);
    theme->window_style.normal.border.radius = 8;
    theme->window_style.normal.shadow_count = 0;
    
    // Label style
    theme->label_style.normal.text_color = theme->on_surface;
    
    // Input style
    theme->input_style.normal.background = color_rgb(0x20, 0x20, 0x20);
    theme->input_style.normal.border.width = 1;
    theme->input_style.normal.border.color = color_rgb(0x60, 0x60, 0x60);
    theme->input_style.normal.border.radius = 4;
    theme->input_style.focused.border.color = theme->accent;
}

void theme_apply(theme_t *theme) {
    current_theme = theme;
    
    // Update all widgets to new theme
    for (widget_t *w = widget_list_head; w; w = w->next) {
        w->theme = theme;
        widget_invalidate(w);
    }
}

theme_t *theme_get_current(void) {
    return current_theme;
}
