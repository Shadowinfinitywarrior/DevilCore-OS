#ifndef DEVILCORE_DEVILUI_H
#define DEVILCORE_DEVILUI_H

#include <stdbool.h>
#include <stdint.h>

#include "compositor.h"
#include "font.h"

// Forward declarations
struct widget;
struct painter;
struct theme;

// Widget types
typedef enum {
    WIDGET_BASE,
    WIDGET_WINDOW,
    WIDGET_BUTTON,
    WIDGET_LABEL,
    WIDGET_TEXTFIELD,
    WIDGET_TEXTAREA,
    WIDGET_CHECKBOX,
    WIDGET_RADIO,
    WIDGET_SLIDER,
    WIDGET_PROGRESS,
    WIDGET_LISTVIEW,
    WIDGET_TREEVIEW,
    WIDGET_TABVIEW,
    WIDGET_SCROLLVIEW,
    WIDGET_DROPDOWN,
    WIDGET_MENUBAR,
    WIDGET_TOOLBAR,
    WIDGET_STATUSBAR,
    WIDGET_DIALOG,
    WIDGET_CANVAS,
    WIDGET_IMAGE,
    WIDGET_SEPARATOR,
    WIDGET_SPACER
} widget_type_t;

// Layout types
typedef enum {
    LAYOUT_NONE,       // Absolute positioning
    LAYOUT_VERTICAL,   // Stack vertically
    LAYOUT_HORIZONTAL, // Stack horizontally
    LAYOUT_GRID,       // Grid layout
    LAYOUT_FLOW,       // Flow/wrap layout
    LAYOUT_ANCHOR      // Anchor-based
} layout_type_t;

// Size policy
typedef enum {
    SIZE_FIXED,        // Fixed size
    SIZE_MINIMUM,      // At least minimum
    SIZE_PREFERRED,    // Preferred size
    SIZE_EXPANDING,    // Take all available space
    SIZE_IGNORED       // Size ignored in layout
} size_policy_t;

// Text alignment
typedef enum {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT,
    TEXT_ALIGN_JUSTIFY
} text_align_t;

// Gradient
typedef struct gradient {
    color_t start;
    color_t end;
    float angle;        // 0=horizontal, 90=vertical
    bool radial;
} gradient_t;

// Border
typedef struct border {
    uint32_t width;
    color_t color;
    uint32_t radius;    // Corner radius
    bool dashed;
} border_t;

// Shadow
typedef struct shadow {
    int32_t offset_x, offset_y;
    uint32_t blur;
    uint32_t spread;
    color_t color;
    bool inset;
} shadow_t;

// Widget style state (per state: normal, hover, pressed, etc.)
typedef struct widget_style_state {
    // Background
    bool has_gradient;
    color_t background;
    gradient_t gradient;
    
    // Border
    border_t border;
    
    // Shadow (multiple shadows)
    shadow_t shadows[4];
    uint32_t shadow_count;
    
    // Text
    color_t text_color;
    font_t *font;
    text_align_t text_align;
    
    // Opacity
    float opacity;
    
    // Blur effect
    float blur_radius;
    
    // Icon
    color_t icon_color;
} widget_style_state_t;

// Complete widget style (all states)
typedef struct widget_style {
    widget_style_state_t normal;
    widget_style_state_t hover;
    widget_style_state_t pressed;
    widget_style_state_t focused;
    widget_style_state_t disabled;
    widget_style_state_t checked;   // For checkboxes
    
    // Transitions (animation)
    uint32_t transition_duration_ms;
    uint8_t transition_easing;
} widget_style_t;

// Theme
typedef struct theme {
    char name[64];
    
    // Base colors
    color_t accent;
    color_t background;
    color_t surface;
    color_t on_background;
    color_t on_surface;
    color_t error;
    color_t success;
    color_t warning;
    
    // Pre-built styles for each widget type
    widget_style_t button_style;
    widget_style_t input_style;
    widget_style_t window_style;
    widget_style_t label_style;
    widget_style_t checkbox_style;
    widget_style_t slider_style;
    widget_style_t listview_style;
    widget_style_t menubar_style;
    widget_style_t tooltip_style;
    widget_style_t dialog_style;
} theme_t;

// Event types
typedef enum {
    EVENT_NONE,
    EVENT_MOUSE_PRESS,
    EVENT_MOUSE_RELEASE,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_ENTER,
    EVENT_MOUSE_LEAVE,
    EVENT_MOUSE_SCROLL,
    EVENT_KEY_PRESS,
    EVENT_KEY_RELEASE,
    EVENT_TEXT_INPUT,
    EVENT_FOCUS_IN,
    EVENT_FOCUS_OUT,
    EVENT_WINDOW_CLOSE,
    EVENT_WINDOW_RESIZE,
    EVENT_PAINT,
    EVENT_TIMER,
    EVENT_ANIMATION_COMPLETE
} event_type_t;

// Event structure
typedef struct event {
    event_type_t type;
    uint64_t timestamp;
    uint32_t widget_id;
    
    union {
        struct {
            int32_t x, y;
            uint8_t button;
            uint8_t clicks;
        } mouse;
        
        struct {
            int32_t dx, dy;
        } scroll;
        
        struct {
            uint32_t keycode;
            uint32_t scancode;
            uint8_t modifiers;
            bool repeat;
        } key;
        
        struct {
            uint32_t codepoint;
        } text;
        
        struct {
            uint32_t width, height;
        } resize;
        
        struct {
            uint32_t timer_id;
        } timer;
    };
} event_t;

// Painter (drawing context)
typedef struct painter {
    // Target surface
    uint32_t *pixels;
    uint32_t width, height, stride;
    
    // Clip region
    rect_t clip;
    
    // Transform stack
    struct {
        int32_t tx, ty;
        float sx, sy;
    } transform_stack[32];
    uint32_t transform_depth;
    
    // Current state
    color_t fill_color;
    color_t stroke_color;
    uint32_t stroke_width;
    float opacity;
    font_t *current_font;
} painter_t;

// Virtual function table for widget subclassing
typedef struct widget_vtable {
    void (*paint)(struct widget *w, painter_t *p);
    void (*layout)(struct widget *w);
    void (*measure)(struct widget *w);
    bool (*hit_test)(struct widget *w, int32_t x, int32_t y);
    struct widget *(*find_child)(struct widget *w, int32_t x, int32_t y);
    void (*destroy)(struct widget *w);
    void (*on_event)(struct widget *w, event_t *event);
} widget_vtable_t;

// Widget base structure
typedef struct widget {
    widget_type_t type;
    uint32_t id;
    char name[64];
    
    // Geometry
    int32_t x, y;
    uint32_t width, height;
    uint32_t min_width, min_height;
    uint32_t max_width, max_height;
    uint32_t preferred_width, preferred_height;
    
    // Layout
    layout_type_t layout;
    size_policy_t h_policy, v_policy;
    uint32_t margin_top, margin_right, margin_bottom, margin_left;
    uint32_t padding_top, padding_right, padding_bottom, padding_left;
    uint32_t spacing;
    
    // State
    bool visible;
    bool enabled;
    bool focused;
    bool hovered;
    bool pressed;
    
    // Style
    widget_style_t *style;
    theme_t *theme;
    
    // Surface for rendering (if needed)
    surface_t *surface;
    
    // Event handlers
    void (*on_paint)(struct widget *w, painter_t *p);
    void (*on_resize)(struct widget *w, uint32_t width, uint32_t height);
    void (*on_mouse_press)(struct widget *w, int32_t x, int32_t y, uint8_t button);
    void (*on_mouse_release)(struct widget *w, int32_t x, int32_t y, uint8_t button);
    void (*on_mouse_move)(struct widget *w, int32_t x, int32_t y);
    void (*on_mouse_enter)(struct widget *w);
    void (*on_mouse_leave)(struct widget *w);
    void (*on_key_press)(struct widget *w, uint32_t key, uint32_t modifiers);
    void (*on_key_release)(struct widget *w, uint32_t key, uint32_t modifiers);
    void (*on_focus_in)(struct widget *w);
    void (*on_focus_out)(struct widget *w);
    void (*on_scroll)(struct widget *w, int32_t dx, int32_t dy);
    
    // Custom data
    void *user_data;
    
    // Tree structure
    struct widget *parent;
    struct widget **children;
    uint32_t child_count;
    uint32_t child_capacity;
    
    // Virtual function table
    widget_vtable_t *vtable;
    
    // Linked list for event loop
    struct widget *next, *prev;
} widget_t;

// DevilUI API
void devilui_init(void);
void devilui_shutdown(void);

// Widget management
widget_t *widget_create(widget_type_t type, const char *name, 
                         int32_t x, int32_t y, uint32_t w, uint32_t h);
void widget_destroy(widget_t *w);
void widget_add_child(widget_t *parent, widget_t *child);
void widget_remove_child(widget_t *parent, widget_t *child);
void widget_show(widget_t *w);
void widget_hide(widget_t *w);
void widget_set_focus(widget_t *w);
void widget_clear_focus(widget_t *w);
void widget_invalidate(widget_t *w);  // Mark for redraw
void widget_layout(widget_t *w);
void widget_paint(widget_t *w, painter_t *p);

// Layout helpers
void widget_set_layout(widget_t *w, layout_type_t layout);
void widget_set_size_policy(widget_t *w, size_policy_t h, size_policy_t v);
void widget_set_margins(widget_t *w, uint32_t top, uint32_t right, 
                         uint32_t bottom, uint32_t left);
void widget_set_padding(widget_t *w, uint32_t top, uint32_t right,
                         uint32_t bottom, uint32_t left);

// Hit testing
widget_t *widget_hit_test(widget_t *root, int32_t x, int32_t y);
bool widget_contains(widget_t *w, int32_t x, int32_t y);

// Painter API
void painter_init(painter_t *p, surface_t *surf);
void painter_save(painter_t *p);
void painter_restore(painter_t *p);
void painter_translate(painter_t *p, int32_t x, int32_t y);
void painter_scale(painter_t *p, float sx, float sy);
void painter_clip_rect(painter_t *p, rect_t *rect);
void painter_set_fill(painter_t *p, color_t color);
void painter_set_stroke(painter_t *p, color_t color, uint32_t width);
void painter_set_opacity(painter_t *p, float opacity);
void painter_set_font(painter_t *p, font_t *font);

// Drawing primitives
void painter_fill_rect(painter_t *p, rect_t *r);
void painter_stroke_rect(painter_t *p, rect_t *r);
void painter_fill_round_rect(painter_t *p, rect_t *r, uint32_t radius);
void painter_stroke_round_rect(painter_t *p, rect_t *r, uint32_t radius);
void painter_fill_circle(painter_t *p, int32_t cx, int32_t cy, uint32_t radius);
void painter_draw_line(painter_t *p, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void painter_draw_text(painter_t *p, int32_t x, int32_t y, const char *text);
void painter_draw_text_rect(painter_t *p, rect_t *r, const char *text, text_align_t align);

// Theme API
void theme_apply(theme_t *theme);
theme_t *theme_get_current(void);
void theme_init_default(theme_t *theme);

// Built-in themes
extern theme_t theme_dark;
extern theme_t theme_light;
extern theme_t theme_catppuccin;
extern theme_t theme_nord;

// Event loop
void event_loop_init(void);
void event_loop_run(void);
void event_loop_quit(void);
void event_push(event_t *event);
bool event_poll(event_t *event);
void event_dispatch(event_t *event);

// Animation system
typedef enum {
    EASING_LINEAR,
    EASING_EASE_IN,
    EASING_EASE_OUT,
    EASING_EASE_IN_OUT,
    EASING_BOUNCE,
    EASING_ELASTIC
} easing_t;

typedef struct animation {
    uint64_t start_time;
    uint64_t duration_ms;
    easing_t easing;
    bool active;
    bool loop;
    
    float *target_float;
    int32_t *target_int;
    color_t *target_color;
    
    union {
        struct { float from, to; } f;
        struct { int32_t from, to; } i;
        struct { color_t from, to; } c;
    } values;
    
    void (*on_complete)(struct animation *anim, void *data);
    void *callback_data;
    
    struct animation *next;
} animation_t;

void animation_init(void);
animation_t *animate_float(float *target, float to, uint32_t duration_ms, easing_t easing);
animation_t *animate_int(int32_t *target, int32_t to, uint32_t duration_ms, easing_t easing);
animation_t *animate_color(color_t *target, color_t to, uint32_t duration_ms, easing_t easing);
void animation_tick(uint64_t current_time_ms);
void animation_stop(animation_t *anim);

// Specific widget constructors
widget_t *window_create(const char *title, int32_t x, int32_t y, 
                         uint32_t w, uint32_t h);
widget_t *button_create(const char *label, widget_t *parent);
widget_t *label_create(const char *text, widget_t *parent);
widget_t *textfield_create(widget_t *parent);
widget_t *slider_create(uint32_t min, uint32_t max, uint32_t value, widget_t *parent);
widget_t *progressbar_create(uint32_t min, uint32_t max, uint32_t value, widget_t *parent);
widget_t *checkbox_create(const char *label, bool checked, widget_t *parent);
widget_t *scrollview_create(widget_t *parent);

extern widget_t *focused_widget;
extern widget_t *hovered_widget;

#endif // DEVILCORE_DEVILUI_H
